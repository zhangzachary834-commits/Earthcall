# Unified Brush System Documentation

## Overview

The Unified Brush System is a powerful, professional-grade painting system that provides advanced brush capabilities for both 2D and 3D painting within the Earthcall application. This system unifies the brush functionality across different painting modes, ensuring consistency and providing a rich set of features typically found in professional digital art software.

## Architecture

### Core Components

1. **BrushSystem Class** (`src/Rendering/BrushSystem.hpp/cpp`)
   - Central brush engine that handles all brush operations
   - Manages brush types, dynamics, pressure simulation, and layer system
   - Provides unified API for both 2D and 3D painting

2. **Zone Integration** (`src/ZonesOfEarth/Zone.hpp/cpp`)
   - Enhanced 2D painting with advanced brush features
   - Maintains backward compatibility with legacy stroke system
   - Integrates BrushSystem for professional 2D painting

3. **Object Integration** (`src/Form/Object/Object.hpp/cpp`)
   - Enhanced 3D face painting with advanced brush features
   - Uses BrushSystem for texture painting on 3D objects
   - Provides layer system and advanced effects

4. **Game UI Integration** (`src/Core/Game.hpp/cpp`)
   - Unified UI controls for both 2D and 3D brush systems
   - Advanced settings panels for professional brush control
   - Preset management and real-time parameter adjustment

## Key Features

### 🎨 Brush Types

#### Normal Brush
- Standard painting with customizable opacity and flow
- Smooth, predictable strokes
- Ideal for base colors and detailed work

#### Airbrush
- Particle-based spraying effect with density control
- Realistic airbrush simulation
- Perfect for gradients and soft transitions

#### Chalk
- Textured brush with randomized opacity for natural look
- Simulates traditional chalk media
- Great for artistic textures and rough surfaces

#### Spray
- Spray can effect with random particle distribution
- High-density particle spraying
- Ideal for graffiti-style effects and texture creation

#### Smudge
- Blends existing colors for realistic smudging
- Color mixing and blending effects
- Perfect for realistic painting and color transitions

#### Clone
- Copies texture from one area to another
- Professional cloning tool with offset control
- Essential for texture repair and pattern creation

### ⚙️ Advanced Dynamics

#### Opacity Control
- **Range**: 0.0 - 1.0
- Controls brush transparency
- Lower values create more transparent strokes

#### Flow Control
- **Range**: 0.0 - 1.0
- Controls paint flow rate
- Affects how quickly paint builds up

#### Spacing Control
- **Range**: 0.01 - 0.5
- Distance between brush dabs in strokes
- Lower values create smoother, more continuous strokes

#### Density Control
- **Range**: 0.1 - 1.0
- Particle density for airbrush effects
- Higher values create more concentrated effects

#### Strength Control
- **Range**: 0.0 - 1.0
- Intensity of smudge and other effects
- Controls the power of blending and mixing

### 🎯 Pressure Simulation

The system simulates pressure sensitivity based on mouse movement speed:

- **Enable Pressure**: Toggle pressure sensitivity on/off
- **Sensitivity**: How much mouse speed affects pressure (0.1 - 5.0)
- **Current Pressure**: Manual pressure override (0.1 - 1.0)

#### How It Works
- Faster mouse movement = lower pressure (thinner strokes)
- Slower mouse movement = higher pressure (thicker strokes)
- Provides natural, responsive painting experience

### 🖌️ Stroke Interpolation

- **Stroke Interpolation**: Smooth stroke rendering between mouse positions
- **Brush Cursor**: Visual cursor showing brush position and size
- **Brush Preview**: Real-time preview of brush settings

#### Benefits
- Eliminates jagged strokes from discrete mouse positions
- Creates smooth, continuous brush strokes
- Professional-quality line rendering

### 🏗️ Layer System

#### Multiple Layers
- Create and manage unlimited texture layers
- Each layer has independent opacity and blend mode
- Non-destructive editing workflow

#### Layer Opacity
- Control individual layer transparency (0.0 - 1.0)
- Real-time opacity adjustment
- Layer-specific transparency control

#### Blend Modes
1. **Normal**: Standard alpha blending
2. **Multiply**: Darkens underlying colors
3. **Screen**: Lightens underlying colors
4. **Overlay**: Contrast enhancement
5. **Add**: Brightens colors
6. **Subtract**: Darkens colors

### 📋 Brush Presets

Pre-configured brush settings for common tasks:

#### Soft Brush
- Gentle painting with low opacity
- Smooth, gradual color application
- Perfect for subtle shading and blending

#### Hard Brush
- Sharp, opaque strokes
- High opacity and flow
- Ideal for bold lines and solid colors

#### Airbrush
- Professional airbrush simulation
- Optimized for gradients and soft effects
- Realistic particle distribution

#### Chalk
- Textured, natural media look
- Randomized opacity for organic feel
- Great for artistic textures

#### Smudge
- Color blending tool
- Optimized for mixing and blending
- Realistic paint mixing simulation

#### Clone
- Texture copying tool
- Professional cloning capabilities
- Essential for texture repair

### ↩️ Undo/Redo System

- **Undo (Ctrl+Z)**: Revert last brush stroke
- **Redo (Ctrl+Y)**: Restore undone stroke
- **Clear History**: Remove all stroke history
- **Stroke-level undo**: Undo individual strokes, not just entire operations

## Usage Guide

### 2D Painting

#### Basic Usage
1. Select "2D Creation" mode in the main toolbar
2. Check "Use Advanced 2D Brush" to enable the new system
3. Click "Advanced Settings" to access professional controls
4. Choose a brush type from the dropdown
5. Adjust basic settings (radius, opacity, flow)
6. Paint on the 2D canvas

#### Advanced Features
1. **Layer Management**:
   - Enable "Use Layers" in advanced settings
   - Use "Add Layer" to create new layers
   - Adjust layer opacity and blend mode
   - Paint on different layers for non-destructive editing

2. **Pressure Simulation**:
   - Enable "Enable Pressure" in advanced settings
   - Adjust sensitivity to your preference
   - Paint with varying mouse speed to see pressure effects

3. **Clone Tool**:
   - Select Clone brush type
   - Enable "Clone Active"
   - Set source point with "Set Source Point" button
   - Paint to copy from source area

### 3D Painting

#### Basic Usage
1. Select "3D" mode in the main toolbar
2. Choose "Face Brush" from the 3D tools
3. Select a brush type from the dropdown
4. Adjust brush settings in the 3D panel
5. Click and drag on cube faces to paint

#### Advanced Features
1. **Brush Dynamics**: Adjust opacity, flow, spacing, density, and strength
2. **Pressure Simulation**: Enable pressure sensitivity for natural strokes
3. **Stroke Interpolation**: Enable for smooth, continuous strokes
4. **Layer System**: Use multiple texture layers on 3D objects
5. **Clone Tool**: Copy textures between different areas

### Keyboard Shortcuts

- **Ctrl+Z**: Undo last stroke
- **Ctrl+Y**: Redo last undone stroke
- **Mouse Drag**: Paint strokes
- **Mouse Click**: Single brush dabs

## Technical Implementation

### BrushSystem Class Structure

```cpp
class BrushSystem {
public:
    enum class BrushType { Normal, Airbrush, Chalk, Spray, Smudge, Clone };
    enum class BlendMode { Normal, Multiply, Screen, Overlay, Add, Subtract };
    
    struct BrushPreset {
        std::string name;
        BrushType type;
        float radius, softness, opacity, flow, spacing, density, strength;
    };
    
    struct Layer {
        std::vector<uint8_t> pixels;
        float opacity;
        BlendMode blendMode;
        std::vector<std::vector<StrokePoint>> strokeHistory;
        std::vector<std::vector<StrokePoint>> undoStack;
        bool visible;
    };
};
```

### Integration Points

#### Zone Integration (2D)
- `Zone::initializeBrushSystem()`: Creates brush system instance
- `Zone::startStroke()`, `continueStroke()`, `endStroke()`: Handle 2D painting
- `Zone::renderArt()`: Renders brush system output

#### Object Integration (3D)
- `Object::paintFaceAdvanced()`: Uses brush system for 3D face painting
- `Object::paintStroke()`: Handles stroke interpolation
- `Object::FaceTexture`: Manages texture layers and compositing

#### Game Integration (UI)
- Advanced brush settings panels
- Preset management
- Real-time parameter adjustment
- Unified controls for both 2D and 3D modes

### Performance Considerations

- **Optimized Rendering**: Efficient pixel manipulation and texture updates
- **Memory Management**: Smart layer management and texture caching
- **Real-time Performance**: Optimized for interactive painting
- **Texture Resolution**: Configurable texture sizes (default 64x64)

## Future Enhancements

### Planned Features
1. **Brush Textures**: Custom brush tip textures
2. **Brush Dynamics**: Size and opacity jitter
3. **Advanced Blending**: More blend modes and custom blending
4. **Brush Masks**: Custom brush shapes and masks
5. **Symmetry Tools**: Radial and linear symmetry
6. **Brush Recording**: Record and replay brush strokes
7. **Export/Import**: Save and load brush presets
8. **Performance Optimization**: GPU acceleration for brush effects

### Technical Improvements
1. **GPU Compute**: Move brush calculations to GPU
2. **Multi-threading**: Parallel brush stroke processing
3. **Memory Optimization**: Better texture memory management
4. **Real-time Preview**: Live brush preview with hardware acceleration

## Troubleshooting

### Common Issues

1. **Brush not responding**:
   - Check if "Use Advanced 2D Brush" is enabled in the 2D Creation panel
   - Verify brush system is initialized
   - Check brush opacity and flow settings

2. **Performance issues**:
   - Reduce texture resolution
   - Disable stroke interpolation for faster painting
   - Limit number of layers

3. **Undo not working**:
   - Ensure stroke state is saved before painting
   - Check if undo stack has content
   - Verify layer system is properly initialized

### Debug Information

The system provides debug output for:
- Stroke start/continue/end events
- Brush system initialization
- Layer operations
- Error conditions

## Conclusion

The Unified Brush System provides a professional-grade painting experience that rivals commercial digital art software. By unifying the brush functionality across 2D and 3D modes, it ensures consistency and provides a rich set of features for creative expression.

The system is designed to be:
- **Extensible**: Easy to add new brush types and effects
- **Performant**: Optimized for real-time interactive painting
- **User-friendly**: Intuitive controls and professional workflow
- **Robust**: Reliable undo/redo and layer management

This unified approach makes the Earthcall application a powerful tool for digital art creation, whether working in 2D or 3D environments. 