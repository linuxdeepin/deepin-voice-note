// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import ListItem from '@tiptap/extension-list-item'
import TaskItem from '@tiptap/extension-task-item'

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
})
