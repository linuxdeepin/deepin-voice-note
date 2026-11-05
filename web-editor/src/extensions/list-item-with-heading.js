// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import ListItem from '@tiptap/extension-list-item'
import TaskItem from '@tiptap/extension-task-item'
import { taskCheckboxIntentKey } from './task-checkbox-intent.js'

// 列表项的块级样式与普通文本块保持一致。标准 ListItem/TaskItem 只允许
// paragraph 作为首块；用户在列表项上选择标题时，setHeading 无法在原位置
// 替换节点，ProseMirror 会拆掉整个列表并把 heading 提升到顶层，造成列表
// 消失和选区漂移。运行时编辑器与保存校验必须共用这组扩展，避免一边允许
// heading、另一边在加载时又判定为非法结构。
export const ListItemWithHeading = ListItem.extend({
  content: '(paragraph|heading) block*',
})

export const TaskItemWithHeading = TaskItem.extend({
  content: '(paragraph|heading) block*',

  addCommands() {
    return {
      ...this.parent?.(),
      // position 可由 checkbox NodeView 明确传入；省略时作用于光标所在待办。
      setTaskChecked: (checked, position) => ({ tr, dispatch }) => {
        if (typeof checked !== 'boolean') return false
        let pos = position
        if (pos === undefined) {
          const { $from } = tr.selection
          for (let depth = $from.depth; depth > 0; depth -= 1) {
            if ($from.node(depth).type.name === this.name) {
              pos = $from.before(depth)
              break
            }
          }
        }
        if (!Number.isInteger(pos) || pos < 0 || pos >= tr.doc.content.size) return false
        const node = tr.doc.nodeAt(pos)
        if (node?.type.name !== this.name) return false
        if (dispatch) {
          tr.setNodeMarkup(pos, undefined, { ...node.attrs, checked })
          const intents = tr.getMeta(taskCheckboxIntentKey) || []
          tr.setMeta(taskCheckboxIntentKey, [...intents, {
            pos, checked, mappingIndex: tr.mapping.maps.length,
          }])
        }
        return true
      },
    }
  },

  addNodeView() {
    const parentNodeView = this.parent()
    return (props) => {
      const nodeView = parentNodeView(props)
      const checkbox = nodeView.dom.firstElementChild.querySelector('input[type="checkbox"]')
      const onChange = (event) => {
        // 只读分支仍由上游处理，包括 onReadOnlyChecked 回调和复选框恢复。
        if (!props.editor.isEditable) return
        event.stopImmediatePropagation()
        const position = props.getPos()
        if (typeof position !== 'number') return
        props.editor.chain().focus(undefined, { scrollIntoView: false })
          .setTaskChecked(checkbox.checked, position).run()
      }
      // 捕获阶段接管编辑态 change；保留上游 NodeView 的渲染、a11y 和更新逻辑。
      checkbox.addEventListener('change', onChange, true)
      const destroy = nodeView.destroy?.bind(nodeView)
      nodeView.destroy = () => {
        checkbox.removeEventListener('change', onChange, true)
        destroy?.()
      }
      return nodeView
    }
  },
})
