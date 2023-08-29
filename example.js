const zed = require('./zed.js')
console.log("Devices", zed.devices)
let cam = new zed.Camera().open()
console.log("cam", cam)
// cloud is a Float32Array of )(vec4) points, normally of size cam.width x cam.height x 4
// the vec4 is x, y, z, w where xyz are the positions (in meters) and w is a packed 4-byte RGBA color
let cloud = cam.cloud
let normals = cam.normals

function info(c, r) {
	const { width, height, cloud, normals } = cam
	let idx = 4 * (c + r*width)

	// get centre pixel:
	let pos = cloud.subarray(idx, idx+3)
	let normal = normals.subarray(idx, idx+3)
	let rgba = new Uint8Array(cloud.buffer, (idx+3)*4, 4)
	console.log(c, r, pos, normal, rgba)
}

if (cam.isOpened()) {
	console.log(cam.grab())

	const { width, height, cloud, normals } = cam
	let c = Math.floor(width/2)
	let r = Math.floor(height/2)
	info(c, r)
	info(0, 0)

}

console.log("done")