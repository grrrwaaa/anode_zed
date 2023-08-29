const zed = require('./zed.js')

const assert = require("assert"),
	fs = require("fs"),
    path = require("path")

const { vec2, vec3, vec4, quat, mat2, mat2d, mat3, mat4} = require("gl-matrix")

// add anode_gl to the module search paths:
module.paths.push(path.resolve(path.join(__dirname, "..", "anode_gl")))

const gl = require('gles3.js'),
	glfw = require('glfw3.js'),
    Window = require("window.js"),
	glutils = require('glutils.js'),
	Shaderman = require('shaderman.js')

console.log("Devices", zed.devices)

// initial config: 
// NEURAL
// depth stabilization 50%
let cam = new zed.Camera().open()
console.log("cam", cam)

let window = new Window({
    width: cam.width, height: cam.height,
})

const shaderman = new Shaderman(gl)
shaderman.create(gl, "normals")
shaderman.create(gl, "points")

const quad_vao = glutils.createVao(gl, glutils.makeQuad())

// create a texture for the normals data:
const cloudtex = glutils.createTexture(gl, { 
    float: true, channels: 4, width: cam.width, height: cam.height
})

const normaltex = glutils.createTexture(gl, { 
    float: true, channels: 4, width: cam.width, height: cam.height
})

const points_vao = glutils.createVao(gl, {
    vertexComponents: 4,
    vertices: cam.cloud,
    normals: cam.normals,
    // colors:
    texCoords: new Float32Array(cam.width * cam.height * 2)
})
// add a buffer for the colors:
{
    points_vao.bind()
    // normalize 0
    // bytestride = 4 x float32 = 16
    // byteoffset = 3 x float32 = 12
    gl.enableVertexAttribArray(3);
    gl.bindBuffer(gl.ARRAY_BUFFER, points_vao.vertexBuffer);
    gl.vertexAttribPointer(3, 4, gl.UNSIGNED_BYTE, 1, 16, 12);
    points_vao.unbind()
}

points_vao.geom.texCoords.forEach((v, i, a) => {
    let idx = Math.floor(i/2)
    let y = Math.floor(idx / cam.width) / cam.height
    let x = (idx % cam.width) / cam.width
    a[i] = (i % 2) ? y : x;
})

function info(c, r) {
	const { width, height, cloud, normals } = cam
	let idx = 4 * (c + r*width)

	// get centre pixel:
	let pos = cloud.subarray(idx, idx+3)
	let normal = normals.subarray(idx, idx+3)
	let rgba = new Uint8Array(cloud.buffer, (idx+3)*4, 4)
	console.log(c, r, pos, normal, rgba)
}

window.draw = function() {
	let { t, dt } = this;

    // const { width, height, cloud, normals } = cam
	// let c = Math.floor(width/2)
	// let r = Math.floor(height/2)
	// info(c, r)

	let viewmatrix = mat4.create();
	let projmatrix = mat4.create();
	let modelmatrix = mat4.create();


    if (cam.isOpened()) {
        // runtime config
        // disable fill
        // confidence 0-5%
        // texture confidence 95%
        cam.grab()
        const { width, height, cloud, normals } = cam
    }
    
    let dim = glfw.getFramebufferSize(this.window)
    mat4.perspective(projmatrix, Math.PI * 0.5, dim[0] / dim[1], 0.3, 10)
    let a = t
    let at = [0, 0, -3]
    let eye = [at[0] + Math.sin(a), at[1], at[2] + Math.cos(a)]

    mat4.lookAt(viewmatrix, eye, at, [0, 1, 0])

    gl.viewport(0, 0, dim[0], dim[1]);
	gl.clearColor(0, 0, 0, 1);
	gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    cloudtex.data = cam.cloud
    cloudtex.bind(1).submit()

    normaltex.data = cam.normals
    normaltex.bind(0).submit()

    if (0) {

        shaderman.shaders.normals.begin()
        .uniform("u_tex_normals", 0)
        .uniform("u_tex_cloud", 1)
        quad_vao.bind().draw()
    } else {
        shaderman.shaders.points.begin()
        .uniform("u_tex_normals", 0)
        .uniform("u_tex_cloud", 1)
        .uniform("u_modelmatrix", modelmatrix)
        .uniform("u_viewmatrix", viewmatrix)
        .uniform("u_projmatrix", projmatrix)
        // quad_vao.bind().draw()

        points_vao.bind().submit().drawPoints()
    }

    //console.log(1/dt)
}

Window.animate()