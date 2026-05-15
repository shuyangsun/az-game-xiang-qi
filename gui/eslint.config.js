//  @ts-check

import { tanstackConfig } from '@tanstack/eslint-config'
import reactCompiler from 'eslint-plugin-react-compiler'

export default [
  ...tanstackConfig,
  {
    plugins: {
      'react-compiler': reactCompiler,
    },
    rules: {
      'import/no-cycle': 'off',
      'import/order': 'off',
      'sort-imports': 'off',
      '@typescript-eslint/array-type': 'off',
      '@typescript-eslint/require-await': 'off',
      'pnpm/json-enforce-catalog': 'off',
      'react-compiler/react-compiler': 'error',
    },
  },
  {
    ignores: ['eslint.config.js', 'prettier.config.js', 'public/wasm/xq.js'],
  },
]
