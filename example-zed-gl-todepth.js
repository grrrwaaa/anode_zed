const zed = require('./zed.js')

const assert = require("assert"),
	fs = require("fs"),
    path = require("path")

const { vec2, vec3, vec4, quat, mat2, mat2d, mat3, mat4} = require("gl-matrix")

//2.8mm pitch
const options = {
    run: true,
    tex: 1
}

// what we want here is an orthographic projection, whose dimensions match the screen (in meters)
// and the view is oriented such that Y is up from ground level, Z is out from the screen, and X is on the plane of the screen

// set the height of the screen in meters (this defines the height in meters of data that will be rendered)
let ortho_height_m = 2
// the view will be oriented to the screen
// near & far set the effective minimum and maximum distance from the screen (in meters) that data is rendered:
let near = 0, far = 5 
// the camera position relative to the screen
// (x position along screen width, y up from screen base, z meters in front of the screen)
let camera_pos = [0, 1.5, 0]
let camera_rotation = 0

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
shaderman.create(gl, "show")
shaderman.create(gl, "points_depth")

const quad_vao = glutils.createVao(gl, glutils.makeQuad())

// create a texture for the normals data:
const cloudtex = glutils.createTexture(gl, { 
    float: true, channels: 4, width: cam.width, height: cam.height
})

const normaltex = glutils.createTexture(gl, { 
    float: true, channels: 4, width: cam.width, height: cam.height
})

const flat_fbo = glutils.makeGbuffer(gl, cam.width, cam.height, [
    {}
]);

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

    // also fix up normals, because they use a stride of 4 floats, not 3:
    gl.bindBuffer(gl.ARRAY_BUFFER, points_vao.normalBuffer);
    gl.vertexAttribPointer(1, 3, gl.FLOAT, 0, 16, 0);
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



let axisy = [0, 1, 0] // some basic default
let modelmatrix_cam = mat4.create();

window.draw = function() {
	let { t, dt } = this;

	let viewmatrix = mat4.create();
	let projmatrix = mat4.create();

    if (cam.isOpened()) {
        // runtime config
        // disable fill
        // confidence 0-5%
        // texture confidence 95%
        cam.grab()

        // send to GPU
        cloudtex.data = cam.cloud
        cloudtex.bind(1).submit()
        normaltex.data = cam.normals
        normaltex.bind(0).submit()

        if (t < 10) {
            // in the first 10 seconds, use accelerometer data to adjust our orientation
        
            //console.log(cam.acceleration)
            let a = vec3.clone(cam.acceleration)
            vec3.normalize(a, a)
            vec3.lerp(axisy, axisy, a, 0.1)
            vec3.normalize(axisy, axisy)

            let axisz = vec3.cross(vec3.create(), axisy, [axisy[2], axisy[0], axisy[1]])
            vec3.normalize(axisz, axisz)
            let axisx = vec3.cross(vec3.create(), axisy, axisz)
            vec3.normalize(axisz, axisz)

            let cammatrix = mat4.create()
            mat4.set(cammatrix, 
                axisx[0], axisy[0], axisz[0], 0.,
                axisx[1], axisy[1], axisz[1], 0.,
                axisx[2], axisy[2], axisz[2], 0.,
                0, 0, 0, 1);

            // this process has probably left our axisx and axisz not properly rotated to the XZ plane anymore
            // I'd like to rotateY to ensure that axisx is maximal in +x
            // get the rotation around Y that would make axisx.x maximal:
            let m = mat4.create()
            mat4.fromRotation(m, Math.acos(cammatrix[0]), [0, 1, 0])
            mat4.multiply(cammatrix, m, cammatrix)

            // now generate our camera's modelmatrix
            mat4.identity(modelmatrix_cam)
            mat4.translate(modelmatrix_cam, modelmatrix_cam, camera_pos)
            mat4.rotateY(modelmatrix_cam, modelmatrix_cam, camera_rotation)
            mat4.multiply(modelmatrix_cam, modelmatrix_cam, cammatrix)
        }
    }
    

    flat_fbo.begin() 
    {
        let dim = [flat_fbo.width, flat_fbo.height]
        let aspect = dim[0]/dim[1]
        mat4.ortho(projmatrix, 
            // x axis centered on screen:
            -aspect*ortho_height_m/2, aspect*ortho_height_m/2, 
            // y axis up from bottom of screen:
            0, ortho_height_m, 
            near, far)
        // view matrix looking out of screen
        mat4.identity(viewmatrix)

        gl.viewport(0, 0, dim[0], dim[1]);
        gl.clearColor(0, 0, 0, 1);
        gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

        shaderman.shaders.points_depth.begin()
        .uniform("u_modelmatrix", modelmatrix_cam)
        .uniform("u_viewmatrix", viewmatrix)
        .uniform("u_projmatrix", projmatrix)
        .uniform("u_near", near)
        .uniform("u_far", far)
        .uniform("u_pointsize", dim[0] * 0.005)
        points_vao.bind().submit().drawPoints()
    }
    flat_fbo.end()
    
    let dim = glfw.getFramebufferSize(this.window)
    gl.viewport(0, 0, dim[0], dim[1]);
	gl.clearColor(0, 0, 0, 1);
	gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    flat_fbo.bind()
    shaderman.shaders.show.begin()
    quad_vao.bind().draw()
    
}

const mouse = {
    down: 0,
    pos: [0, 0]
}

window.onpointerbutton = function(button, action, mods) {
    console.log(button, action, mods)
    mouse.down = action;
}

window.onpointermove = function(x, y) {
    mouse.pos = [x, y]
    if (mouse.down) {
        camera_rotation = x*Math.PI
        camera_distance = y*4
    }
}

window.onkey = function(key, scan, down, mod) {
    if (down) {
        switch(key) {
            case 32: {
                options.run = !options.run;
            } break;
            
            case 84: {
                options.tex = !options.tex;
            } break;
            default: console.log(key, scan, down, mod);
        }
        
    }
}

Window.animate()