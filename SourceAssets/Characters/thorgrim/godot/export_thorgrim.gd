extends SceneTree

const SOURCE_OBJ := "res://../exports/thorgrim_v0.obj"
const SOURCE_MTL := "res://../exports/thorgrim_v0.mtl"
const OUTPUT_GLB := "res://../exports/thorgrim_v0.glb"
const MATERIAL_ORDER := [
	"Night",
	"Steel",
	"Ash",
	"Bone",
	"Hide",
	"Fur",
	"Skin",
	"Wood",
	"Cloth",
]


func _initialize() -> void:
	call_deferred("_export")


func _export() -> void:
	var obj_path := ProjectSettings.globalize_path(SOURCE_OBJ)
	var mtl_path := ProjectSettings.globalize_path(SOURCE_MTL)
	var output_path := ProjectSettings.globalize_path(OUTPUT_GLB)

	var palette := _read_palette(mtl_path)
	var obj_data := _read_obj(obj_path)
	if palette.is_empty() or obj_data.is_empty():
		quit(1)
		return

	var mesh := _build_mesh(obj_data, palette)
	if mesh == null or mesh.get_surface_count() == 0:
		_fail("OBJ conversion produced no mesh surfaces")
		return

	var model_root := Node3D.new()
	model_root.name = "Thorgrim_v0"
	var mesh_instance := MeshInstance3D.new()
	mesh_instance.name = "Thorgrim_v0_Mesh"
	mesh_instance.mesh = mesh
	model_root.add_child(mesh_instance)
	mesh_instance.owner = model_root

	var export_document := GLTFDocument.new()
	var export_state := GLTFState.new()
	var result := export_document.append_from_scene(model_root, export_state)
	if result != OK:
		model_root.free()
		_fail("GLTF scene conversion failed with error %d" % result)
		return

	result = export_document.write_to_filesystem(export_state, output_path)
	model_root.free()
	if result != OK:
		_fail("GLB write failed with error %d" % result)
		return

	# Re-open the output through Godot's glTF importer before declaring success.
	var verify_document := GLTFDocument.new()
	var verify_state := GLTFState.new()
	result = verify_document.append_from_file(output_path, verify_state)
	if result != OK:
		_fail("Generated GLB could not be read back (error %d)" % result)
		return
	var verify_scene := verify_document.generate_scene(verify_state)
	if verify_scene == null:
		_fail("Generated GLB did not reconstruct a scene")
		return
	var verified_mesh_count := _count_mesh_instances(verify_scene)
	verify_scene.free()
	if verified_mesh_count < 1:
		_fail("Generated GLB contains no MeshInstance3D nodes")
		return

	print(
		"Thorgrim GLB export complete: %d source objects, %d vertices, %d faces, %d palette surfaces, %d verified meshes"
		% [
			int(obj_data["object_count"]),
			obj_data["vertices"].size(),
			int(obj_data["face_count"]),
			mesh.get_surface_count(),
			verified_mesh_count,
		]
	)
	print(output_path)
	quit(0)


func _read_palette(path: String) -> Dictionary:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		_fail("Could not open MTL file: %s" % path)
		return {}

	var palette := {}
	var current_material := ""
	while not file.eof_reached():
		var line := file.get_line().strip_edges()
		if line.begins_with("newmtl "):
			current_material = line.substr(7).strip_edges()
		elif line.begins_with("Kd ") and not current_material.is_empty():
			var parts := line.split(" ", false)
			if parts.size() >= 4:
				palette[current_material] = Color(
					float(parts[1]),
					float(parts[2]),
					float(parts[3]),
					1.0
				)
	file.close()

	for material_name in MATERIAL_ORDER:
		if not palette.has(material_name):
			_fail("MTL palette is missing material %s" % material_name)
			return {}
	return palette


func _read_obj(path: String) -> Dictionary:
	var file := FileAccess.open(path, FileAccess.READ)
	if file == null:
		_fail("Could not open OBJ file: %s" % path)
		return {}

	var vertices := []
	var faces_by_material := {}
	var current_material := ""
	var object_count := 0
	var face_count := 0

	while not file.eof_reached():
		var line := file.get_line().strip_edges()
		if line.is_empty() or line.begins_with("#"):
			continue
		var parts := line.split(" ", false)
		if parts.is_empty():
			continue
		match parts[0]:
			"v":
				if parts.size() < 4:
					file.close()
					_fail("Malformed OBJ vertex: %s" % line)
					return {}
				vertices.append(Vector3(float(parts[1]), float(parts[2]), float(parts[3])))
			"o":
				object_count += 1
			"usemtl":
				if parts.size() < 2:
					file.close()
					_fail("Malformed OBJ material assignment: %s" % line)
					return {}
				current_material = parts[1]
				if not faces_by_material.has(current_material):
					faces_by_material[current_material] = []
			"f":
				if current_material.is_empty() or parts.size() < 4:
					file.close()
					_fail("Malformed OBJ face or missing material: %s" % line)
					return {}
				var polygon := PackedInt32Array()
				for part_index in range(1, parts.size()):
					var raw_index := int(parts[part_index].get_slice("/", 0))
					var resolved_index := raw_index - 1 if raw_index > 0 else vertices.size() + raw_index
					if resolved_index < 0 or resolved_index >= vertices.size():
						file.close()
						_fail("OBJ face references invalid vertex %d" % raw_index)
						return {}
					polygon.append(resolved_index)
				var material_faces: Array = faces_by_material[current_material]
				material_faces.append(polygon)
				faces_by_material[current_material] = material_faces
				face_count += 1
	file.close()

	if vertices.is_empty() or face_count == 0:
		_fail("OBJ contained no usable geometry")
		return {}
	return {
		"vertices": vertices,
		"faces_by_material": faces_by_material,
		"object_count": object_count,
		"face_count": face_count,
	}


func _build_mesh(obj_data: Dictionary, palette: Dictionary) -> ArrayMesh:
	var mesh := ArrayMesh.new()
	mesh.resource_name = "Thorgrim_v0_Mesh"
	var source_vertices: Array = obj_data["vertices"]
	var faces_by_material: Dictionary = obj_data["faces_by_material"]

	for material_name in MATERIAL_ORDER:
		if not faces_by_material.has(material_name):
			continue
		var surface_tool := SurfaceTool.new()
		surface_tool.begin(Mesh.PRIMITIVE_TRIANGLES)
		var material := StandardMaterial3D.new()
		material.resource_name = material_name
		material.albedo_color = palette[material_name]
		material.metallic = 0.0
		material.roughness = 1.0
		surface_tool.set_material(material)

		var material_faces: Array = faces_by_material[material_name]
		for polygon in material_faces:
			# OBJ is authored X-forward/Y-right/Z-up. Godot/glTF is
			# X-right/Y-up/-Z-forward. This basis conversion reflects the
			# winding, so the triangle order is reversed at the same time.
			var first := _to_gltf_basis(source_vertices[polygon[0]])
			for corner in range(1, polygon.size() - 1):
				var second := _to_gltf_basis(source_vertices[polygon[corner + 1]])
				var third := _to_gltf_basis(source_vertices[polygon[corner]])
				var normal := (second - first).cross(third - first).normalized()
				for vertex in [first, second, third]:
					surface_tool.set_normal(normal)
					surface_tool.add_vertex(vertex)

		surface_tool.index()
		var surface_index := mesh.get_surface_count()
		surface_tool.commit(mesh)
		mesh.surface_set_name(surface_index, material_name)
	return mesh


func _to_gltf_basis(source: Vector3) -> Vector3:
	return Vector3(source.y, source.z, -source.x)


func _count_mesh_instances(node: Node) -> int:
	var count := 1 if node is MeshInstance3D else 0
	for child in node.get_children():
		count += _count_mesh_instances(child)
	return count


func _fail(message: String) -> void:
	push_error(message)
	quit(1)
