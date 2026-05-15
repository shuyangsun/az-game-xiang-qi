export interface XqWasmConfig {
  wasmUrl?: string
  jsUrl?: string
}

let wasmModulePromise: Promise<any> | null = null

export async function loadXqWasm(config: XqWasmConfig = {}) {
  if (wasmModulePromise) {
    return wasmModulePromise
  }

  const jsUrl = config.jsUrl || '/wasm/xq.js'
  const wasmUrl = config.wasmUrl || '/wasm/xq.wasm'

  wasmModulePromise = new Promise((resolve, reject) => {
    // If running in SSR/Node environment, this won't work directly,
    // but the engine is meant to be client-side only.
    if (typeof window === 'undefined') {
      return reject(new Error('WASM module must be loaded in the browser.'))
    }

    const script = document.createElement('script')
    script.src = jsUrl
    script.onload = async () => {
      // @ts-ignore - Emscripten creates this global
      if (typeof window.createXqModule === 'function') {
        try {
          // @ts-ignore - Emscripten creates this global
          const Module = await window.createXqModule({
            locateFile: (path: string, prefix: string) => {
              if (path.endsWith('.wasm')) {
                return wasmUrl
              }
              return prefix + path
            },
          })
          resolve(Module)
        } catch (e) {
          reject(e)
        }
      } else {
        reject(new Error('createXqModule is not defined after script load.'))
      }
    }
    script.onerror = reject
    document.head.appendChild(script)
  })

  return wasmModulePromise
}
