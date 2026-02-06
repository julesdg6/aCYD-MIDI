import { defineConfig } from 'vite'

export default defineConfig({
  base: '/aCYD-MIDI/debug-console/',
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    sourcemap: true
  }
})
