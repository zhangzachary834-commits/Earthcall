import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const isEmbed = process.env.BUILD_TARGET === 'embed'
  
  return {
    plugins: [react()],
    build: {
      outDir: isEmbed ? '../py/static/ui' : 'dist',
      emptyOutDir: true,
      rollupOptions: isEmbed ? {
        input: './src/main.jsx',
        output: {
          entryFileNames: 'app.js',
          assetFileNames: 'app.[ext]',
        }
      } : undefined
    }
  }
})
