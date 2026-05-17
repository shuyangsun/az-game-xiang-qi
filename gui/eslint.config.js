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
      // Modern React only: every entry below points to a current
      // first-class replacement. Adding to this list should always
      // come with a one-line justification in the message.
      'no-restricted-imports': [
        'error',
        {
          paths: [
            {
              name: 'react',
              importNames: ['memo', 'useCallback', 'useMemo'],
              message:
                'Use React Compiler for memoisation — manual memo/useCallback/useMemo are banned.',
            },
            {
              name: 'react',
              importNames: ['Component', 'PureComponent', 'createRef'],
              message:
                'Class components are legacy. Use a function component with hooks (useRef instead of createRef).',
            },
            {
              name: 'react',
              importNames: ['forwardRef'],
              message:
                'forwardRef is legacy in React 19+. Accept `ref` as a regular prop on the function component.',
            },
            {
              name: 'react',
              importNames: [
                'createElement',
                'cloneElement',
                'Children',
                'isValidElement',
              ],
              message:
                'Element-manipulation APIs are anti-patterns. Use JSX and composition (render props / children-as-prop) instead.',
            },
            {
              name: 'react-dom',
              importNames: [
                'render',
                'hydrate',
                'unmountComponentAtNode',
                'findDOMNode',
              ],
              message:
                'Legacy react-dom API. Use createRoot / hydrateRoot from `react-dom/client` and `root.unmount()`.',
            },
          ],
        },
      ],
      'no-restricted-syntax': [
        'error',
        {
          selector:
            "CallExpression[callee.object.name='React'][callee.property.name=/^(memo|useCallback|useMemo|forwardRef|createElement|cloneElement|isValidElement|createRef)$/]",
          message:
            'Legacy/discouraged React.* namespace API. Use the modern equivalent (React Compiler / ref-as-prop / JSX).',
        },
        {
          selector:
            "ClassDeclaration[superClass.name=/^(Component|PureComponent)$/]",
          message:
            'Class components are legacy. Use a function component with hooks.',
        },
        {
          selector:
            "ClassDeclaration[superClass.type='MemberExpression'][superClass.object.name='React'][superClass.property.name=/^(Component|PureComponent)$/]",
          message:
            'Class components are legacy. Use a function component with hooks.',
        },
        {
          selector:
            "MemberExpression[object.name='React'][property.name=/^(Children|Component|PureComponent)$/]",
          message:
            'Legacy React.* namespace member. Use composition (for Children) / function components (for Component/PureComponent).',
        },
        {
          selector:
            "CallExpression[callee.object.name='ReactDOM'][callee.property.name=/^(render|hydrate|unmountComponentAtNode|findDOMNode)$/]",
          message:
            'Legacy ReactDOM.* call. Use createRoot / hydrateRoot from `react-dom/client`.',
        },
      ],
    },
  },
  {
    ignores: ['eslint.config.js', 'prettier.config.js', 'public/wasm/xq.js'],
  },
]
