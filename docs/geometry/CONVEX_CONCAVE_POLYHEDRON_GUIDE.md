# Convex/Concave Polyhedron System Guide

## Overview

The polyhedron system has been enhanced to support both convex and concave shapes, giving you much more creative freedom in 3D object creation. This guide explains how to use these new features.

## What's New

### 🔷 Convex/Concave Variants
- **Regular**: Standard convex polyhedrons (tetrahedron, octahedron, dodecahedron, icosahedron)
- **Concave**: Polyhedrons with inward-curved surfaces
- **Star**: Polyhedrons with outward spikes
- **Crater**: Polyhedrons with alternating inward depressions

### 🔷 Advanced Analysis
- **Convexity Detection**: Automatically determines if a polyhedron is convex or concave
- **Face Analysis**: Individual face convexity and area calculations
- **Vertex Curvature**: Mathematical curvature analysis at each vertex
- **Proper Normal Calculation**: Ensures correct lighting for concave faces

## How to Use

### 1. Accessing the Convex/Concave Controls

1. Open the **3D Shape Generator** tool
2. Select **"Polyhedron"** from the Shape dropdown
3. Look for the **"🔷 Convex/Concave Variants"** section

### 2. Choosing a Variant

Use the **"Variant"** dropdown to select:
- **Regular**: Standard convex shapes
- **Concave**: Inward-curved surfaces
- **Star**: Outward spikes
- **Crater**: Alternating depressions

### 3. Adjusting Parameters

Each variant has specific controls:

#### Concave Variant
- **Concavity**: Controls how much vertices are pushed inward (0.1 to 0.8)
  - Lower values = subtle concavity
  - Higher values = deep concavity

#### Star Variant
- **Spike Length**: Controls how far vertices extend outward (0.1 to 1.0)
  - Lower values = short spikes
  - Higher values = long spikes

#### Crater Variant
- **Crater Depth**: Controls depth of depressions (0.1 to 0.5)
  - Lower values = shallow craters
  - Higher values = deep craters

### 4. Creating Objects

1. Select your desired variant and adjust parameters
2. Choose a base polyhedron type (Tetrahedron, Octahedron, etc.)
3. Click in the 3D space to create your object
4. The preview will show your exact shape before creation

## Technical Details

### Convexity Analysis

The system automatically analyzes each polyhedron:

- **Global Convexity**: Whether the entire shape is convex
- **Face Convexity**: Whether individual faces are convex
- **Vertex Curvature**: Mathematical curvature at each vertex point

### Normal Calculation

For concave shapes, the system:
- Detects concave faces automatically
- Calculates proper outward-facing normals
- Ensures correct lighting and rendering

### Performance Considerations

- **Simple shapes** (4-8 faces): Fast rendering
- **Complex shapes** (12+ faces): May affect performance
- **Concave variants**: Slightly more computational overhead
- **Star variants**: Can create very large bounding boxes

## Creative Applications

### Concave Variants
- **Craters and depressions**: Create moon-like surfaces
- **Bowl shapes**: Perfect for containers or decorative objects
- **Organic forms**: Simulate natural erosion or wear

### Star Variants
- **Crystal formations**: Create gem-like objects
- **Spiky creatures**: Design fantasy or sci-fi characters
- **Decorative elements**: Add dramatic visual interest

### Crater Variants
- **Textured surfaces**: Create interesting surface patterns
- **Industrial objects**: Simulate mechanical wear or damage
- **Artistic effects**: Add visual complexity to simple shapes

## Tips and Best Practices

### 1. Start Simple
- Begin with regular polyhedrons to understand the base shapes
- Gradually experiment with concave variants
- Use the preview to see changes in real-time

### 2. Parameter Adjustment
- Make small adjustments to see subtle effects
- Combine different face counts with variants for unique results
- Use the performance warning as a guide for complexity

### 3. Texturing Considerations
- Concave faces may need different texturing approaches
- Star variants create interesting lighting effects
- Crater variants add natural texture variation

### 4. Performance Optimization
- Use simpler variants for frequently rendered objects
- Consider face count when creating many objects
- Monitor performance with complex star variants

## Troubleshooting

### Common Issues

**Shape looks wrong:**
- Check that you've selected the correct variant
- Adjust parameters gradually
- Verify face count is appropriate for the variant

**Performance problems:**
- Reduce face count for complex variants
- Use regular variants for background objects
- Consider using simpler shapes for distant objects

**Lighting issues:**
- The system automatically handles normal calculation
- If lighting looks wrong, try regenerating the object
- Check that you're not using extremely high parameter values

### Getting Help

If you encounter issues:
1. Try different parameter values
2. Switch to regular variants temporarily
3. Check the console for any error messages
4. Restart the application if needed

## Advanced Usage

### Custom Polyhedrons
- Combine custom polyhedrons with concave analysis
- The system will automatically detect convexity
- Use for highly specialized shapes

### Batch Creation
- Create multiple objects with different variants
- Mix regular and concave shapes for variety
- Use for creating complex scenes

### Integration with Other Tools
- Concave polyhedrons work with all existing tools
- Face painting works on all variants
- Collision detection adapts to shape complexity

## Future Enhancements

The convex/concave system is designed to be extensible:
- More variant types may be added
- Advanced parameter controls
- Procedural generation options
- Integration with physics simulation

---

This system opens up entirely new possibilities for 3D object creation, allowing you to move beyond simple geometric shapes into more organic and complex forms. Experiment with different combinations to discover unique and interesting objects! 