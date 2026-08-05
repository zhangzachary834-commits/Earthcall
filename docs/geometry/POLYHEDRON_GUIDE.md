# Polyhedron System Guide

## What's New

The Object system has been upgraded to support polyhedrons (3D shapes with flat faces) instead of just cubes. This means you can now create and work with different types of 3D shapes like tetrahedrons, octahedrons, dodecahedrons, and icosahedrons.

## How to Use Polyhedrons

### 1. Selecting Polyhedron Type
- In the 3D creation mode, you'll see a "Shape" dropdown menu
- Select "Polyhedron" from the list
- Additional buttons will appear for different polyhedron types:
  - **Tetrahedron** (4 faces) - A pyramid with triangular faces
  - **Octahedron** (8 faces) - Two pyramids joined at the base
  - **Dodecahedron** (12 faces) - A complex shape with pentagonal faces
  - **Icosahedron** (20 faces) - A shape with many triangular faces

### 2. Creating Polyhedrons
- Choose your polyhedron type using the buttons
- Click in the 3D space to create the polyhedron
- The shape will appear with the selected number of faces

### 3. Texturing Polyhedrons
- Each face of the polyhedron can be painted individually
- Use the face painting tools to add colors and textures
- The texture system works the same way as with cubes

## Technical Details

### What Changed
- Added `PolyhedronData` structure to store vertex and face information
- Updated collision detection to work with polyhedrons
- Modified drawing system to render arbitrary polyhedrons
- Preserved all existing functionality for cubes and other shapes

### Backward Compatibility
- All existing cube objects will continue to work exactly as before
- No existing code was removed - only commented out where necessary
- The system automatically detects the geometry type and handles it appropriately

### Performance
- Polyhedrons use efficient bounding box collision detection
- Face normals are pre-computed for better rendering performance
- Texture system scales automatically to the number of faces

## Advanced Usage

### Custom Polyhedrons
You can create custom polyhedrons by defining your own vertices and faces:

```cpp
std::vector<glm::vec3> vertices = {
    {0.0f, 0.5f, 0.0f},   // apex
    {-0.5f, -0.5f, -0.5f}, // base corners
    {0.5f, -0.5f, -0.5f},
    {0.5f, -0.5f, 0.5f},
    {-0.5f, -0.5f, 0.5f}
};

std::vector<std::vector<int>> faces = {
    {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1}, // triangular faces
    {1, 4, 3, 2} // base face
};

object.createCustomPolyhedron(vertices, faces);
```

### Getting Information
- `getFaces()` returns the number of faces in the polyhedron
- `getCorners()` returns the number of vertices
- `getPolyhedronData()` gives access to the full polyhedron structure

## Troubleshooting

### Common Issues
1. **Shape not appearing**: Make sure you've selected "Polyhedron" in the shape dropdown
2. **Wrong number of faces**: Check that you've selected the correct polyhedron type
3. **Texturing issues**: Each face needs its own texture - the system will create them automatically

### Performance Tips
- Use simpler polyhedrons (tetrahedron, octahedron) for better performance
- Complex polyhedrons (dodecahedron, icosahedron) have more faces and may be slower
- The collision detection uses bounding boxes, so very complex shapes will still be fast

## Future Enhancements

The polyhedron system is designed to be extensible. Future versions could include:
- More polyhedron types (prisms, pyramids, etc.)
- Dynamic polyhedron generation
- Advanced UV mapping for better texturing
- Subdivision surfaces for smoother shapes 