import { useState, useEffect } from 'react'
import { io } from 'socket.io-client'

const SOCKET_URL = 'http://localhost:5005'

const TOOLS = [
  { id: 'brush', icon: '🖌', label: 'Brush' },
  { id: 'eraser', icon: '🧽', label: 'Eraser' },
  { id: 'select', icon: '⬜', label: 'Select' },
  { id: 'magic', icon: '🪄', label: 'Magic Wand' },
  { id: 'clone', icon: '📋', label: 'Clone' },
]

const COLORS = [
  '#ffffff', '#ff0000', '#00ff00', '#0000ff', '#ffff00', '#00ffff', '#ff00ff', '#000000'
]

function App() {
  const [socket, setSocket] = useState(null)
  const [activeTool, setActiveTool] = useState('brush')
  const [activeColor, setActiveColor] = useState('#ffffff')
  const [connected, setConnected] = useState(false)

  useEffect(() => {
    console.log("Connecting to Earthcall backend...")
    const newSocket = io(SOCKET_URL)
    
    newSocket.on('connect', () => {
      console.log('Connected to Earthcall SocketIO')
      setConnected(true)
    })

    newSocket.on('disconnect', () => {
      console.log('Disconnected from Earthcall SocketIO')
      setConnected(false)
    })

    setSocket(newSocket)
    
    return () => newSocket.close()
  }, [])

  const handleToolSelect = (toolId) => {
    setActiveTool(toolId)
    if (socket) {
      socket.emit('tool_change', { tool: toolId })
    }
  }

  const handleColorSelect = (color) => {
    setActiveColor(color)
    if (socket) {
      socket.emit('color_change', { color })
    }
  }

  return (
    <>
      <div className="glass-panel tool-palette">
        {TOOLS.map(tool => (
          <button
            key={tool.id}
            className={`tool-btn ${activeTool === tool.id ? 'active' : ''}`}
            onClick={() => handleToolSelect(tool.id)}
            title={tool.label}
          >
            {tool.icon}
          </button>
        ))}
      </div>
      
      <div className="glass-panel sidebar">
        <h2>Earthcall Design</h2>
        
        <div className="sidebar-section">
          <h3 style={{fontSize: '0.9rem', marginBottom: '8px', color: 'var(--text-muted)'}}>Colors</h3>
          <div className="color-swatch-container">
            {COLORS.map(color => (
              <div 
                key={color}
                className={`color-swatch ${activeColor === color ? 'active' : ''}`}
                style={{ backgroundColor: color }}
                onClick={() => handleColorSelect(color)}
              />
            ))}
          </div>
        </div>
        
        <hr style={{borderColor: 'rgba(255,255,255,0.1)', margin: '8px 0'}} />
        
        <div className="sidebar-section" style={{flex: 1}}>
          <h3 style={{fontSize: '0.9rem', marginBottom: '8px', color: 'var(--text-muted)'}}>Layers</h3>
          <div className="layer-list">
            <div className="layer-item active">
              <span>Layer 1</span>
              <span>👁️</span>
            </div>
            <div className="layer-item">
              <span>Background</span>
              <span>👁️</span>
            </div>
          </div>
        </div>
        
        <div style={{fontSize: '0.8rem', color: connected ? '#10b981' : '#ef4444'}}>
          {connected ? '● Connected to Engine' : '● Disconnected'}
        </div>
      </div>
    </>
  )
}

export default App
