// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

// 列表行首 Backspace 行为回归。
//
// 产品规则（2026-10-15 确认）：光标在列表项文本行首按 Backspace，与 Shift+Tab
// 同语义 —— 逐级反缩进，一级时退出列表变成普通文本块。旧实现走 core Keymap 的
// joinBackward，会把当前项并进上一个列表项、生成“一个 listItem 多个段落”的结构，
// 导致连续按 Backspace 永远降不到一级、也取消不掉列表属性。
import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { liftActiveListItem, sinkActiveListItem } from '../src/runtime/list-behavior.js'

function defineGlobal(name, value) {
  Object.defineProperty(globalThis, name, { value, writable: true, configurable: true })
}

function setupDom() {
  const window = new Window()
  defineGlobal('window', window)
  defineGlobal('document', window.document)
  defineGlobal('navigator', window.navigator)
  defineGlobal('Node', window.Node)
  defineGlobal('Element', window.Element)
  defineGlobal('HTMLElement', window.HTMLElement)
  defineGlobal('DocumentFragment', window.DocumentFragment)
  defineGlobal('Event', window.Event)
  defineGlobal('KeyboardEvent', window.KeyboardEvent)
  defineGlobal('CustomEvent', window.CustomEvent)
  defineGlobal('MutationObserver', window.MutationObserver)
  defineGlobal('getComputedStyle', window.getComputedStyle.bind(window))
  defineGlobal('requestAnimationFrame', (cb) => setTimeout(cb, 0))
  defineGlobal('cancelAnimationFrame', (id) => clearTimeout(id))
  return window
}

function paragraph(text) {
  return { type: 'paragraph', content: text ? [{ type: 'text', text }] : [] }
}

function listItem(text, extra = [], indentLevel = 0) {
  return {
    type: 'listItem',
    attrs: { dvnIndentLevel: indentLevel },
    content: [paragraph(text), ...extra],
  }
}

function taskItem(text, extra = [], indentLevel = 0) {
  return {
    type: 'taskItem',
    attrs: { checked: false, dvnIndentLevel: indentLevel },
    content: [paragraph(text), ...extra],
  }
}

function list(type, items) {
  return type === 'orderedList'
    ? { type, attrs: { start: 1 }, content: items }
    : { type, content: items }
}

const LIST_CASES = [
  { name: 'bullet list', listType: 'bulletList', makeItem: listItem },
  { name: 'ordered list', listType: 'orderedList', makeItem: listItem },
  { name: 'task list', listType: 'taskList', makeItem: taskItem },
]

function createEditor(window, content) {
  const element = window.document.createElement('div')
  window.document.body.appendChild(element)
  return new Editor({ element, extensions: createTiptapExtensions(), content })
}

function textStartPosition(editor, text) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (found === null && node.isText && node.text === text) found = pos
    return true
  })
  if (found === null) throw new Error(`text not found: ${text}`)
  return found
}

function textEndPosition(editor, text) {
  return textStartPosition(editor, text) + text.length
}

// 同名文本取最后一次出现（重复文本场景：要把选区拉到最后一个匹配项）。
function lastTextEndPosition(editor, text) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (node.isText && node.text === text) found = pos + text.length
    return true
  })
  if (found === null) throw new Error(`text not found: ${text}`)
  return found
}

function isInList(editor) {
  return editor.isActive('listItem') || editor.isActive('taskItem')
}

function markFocused(editor) {
  editor.isFocused = true
}

function pressBackspace(editor, window) {
  markFocused(editor)
  const event = new window.KeyboardEvent('keydown', { key: 'Backspace', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(event)
  return event
}

function backspaceAtTextStart(editor, window, text) {
  markFocused(editor)
  editor.chain().focus().setTextSelection(textStartPosition(editor, text)).run()
  return pressBackspace(editor, window)
}

function pressDelete(editor, window) {
  markFocused(editor)
  const event = new window.KeyboardEvent('keydown', { key: 'Delete', bubbles: true, cancelable: true })
  editor.view.dom.dispatchEvent(event)
  return event
}

function deleteAtTextEnd(editor, window, text) {
  markFocused(editor)
  editor.chain().focus().setTextSelection(textEndPosition(editor, text)).run()
  return pressDelete(editor, window)
}

function listTypeForText(node, text, current = null) {
  if (!node || typeof node !== 'object') return null
  const next = ['bulletList', 'orderedList', 'taskList'].includes(node.type) ? node.type : current
  if (node.type === 'text' && node.text === text) return next
  if (Array.isArray(node.content)) {
    for (const child of node.content) {
      const found = listTypeForText(child, text, next)
      if (found != null) return found
    }
  }
  return null
}

function listItemTypeForText(node, text) {
  let found = null
  const walk = (n, current) => {
    if (!n || typeof n !== 'object' || found !== null) return
    const next = n.type === 'listItem' || n.type === 'taskItem' ? n.type : current
    if (n.type === 'text' && n.text === text) { found = next; return }
    if (Array.isArray(n.content)) n.content.forEach((child) => walk(child, next))
  }
  walk(node, null)
  return found
}

// 文本的视觉层级：结构层级 + dvnIndentLevel，封顶三级；不在列表里返回 0。
function visualDepthForText(node, text, structuralDepth = 0, visualDepth = 0) {
  if (!node || typeof node !== 'object') return null
  const isItem = node.type === 'listItem' || node.type === 'taskItem'
  const nextStructural = isItem ? structuralDepth + 1 : structuralDepth
  const nextVisual = isItem
    ? Math.min(3, nextStructural + (Number(node.attrs?.dvnIndentLevel) || 0))
    : visualDepth
  if (node.type === 'text' && node.text === text) return nextVisual
  if (Array.isArray(node.content)) {
    for (const child of node.content) {
      const found = visualDepthForText(child, text, nextStructural, nextVisual)
      if (found != null) return found
    }
  }
  return null
}

// 收集所有“自身文本等于 text”的列表项的视觉层级（重复文本场景逐项检查）。
function visualDepthsForAll(node, text, structuralDepth = 0, visualDepth = 0, acc = []) {
  if (!node || typeof node !== 'object') return acc
  const isItem = node.type === 'listItem' || node.type === 'taskItem'
  const nextStructural = isItem ? structuralDepth + 1 : structuralDepth
  const nextVisual = isItem
    ? Math.min(3, nextStructural + (Number(node.attrs?.dvnIndentLevel) || 0))
    : visualDepth
  if (isItem) {
    const own = node.content?.find((child) => child.type === 'paragraph')
    if (own?.content?.[0]?.text === text) acc.push(nextVisual)
  }
  if (Array.isArray(node.content)) {
    node.content.forEach((child) => visualDepthsForAll(child, text, nextStructural, nextVisual, acc))
  }
  return acc
}

function textOrder(node, acc = []) {
  if (!node || typeof node !== 'object') return acc
  if (node.type === 'text') acc.push(node.text)
  if (Array.isArray(node.content)) node.content.forEach((child) => textOrder(child, acc))
  return acc
}

// ---------------------------------------------------------------------------
// 一级：取消列表属性
// ---------------------------------------------------------------------------

for (const { name, listType, makeItem } of LIST_CASES) {
  test(`${name}: Backspace at the start of a single level-one item cancels the list`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('唯一项')])],
    })

    backspaceAtTextStart(editor, window, '唯一项')

    const json = editor.getJSON()
    assert.equal(listTypeForText(json, '唯一项'), null, `${name} item should leave the list`)
    assert.equal(listItemTypeForText(json, '唯一项'), null, `${name} item should become a plain block`)
    assert.equal(visualDepthForText(json, '唯一项'), 0, `${name} item should no longer be nested`)
    assert.equal(json.content[0].type, 'paragraph', `${name} list wrapper should be gone`)
    editor.destroy()
  })

  test(`${name}: Backspace at the start of a non-first level-one item cancels only that item`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('第一项'), makeItem('第二项'), makeItem('第三项')])],
    })

    backspaceAtTextStart(editor, window, '第二项')

    const json = editor.getJSON()
    assert.equal(listTypeForText(json, '第一项'), listType, `${name} previous item should stay in the list`)
    assert.equal(listTypeForText(json, '第二项'), null, `${name} active item should leave the list`)
    assert.equal(listItemTypeForText(json, '第二项'), null, `${name} active item should become plain text`)
    assert.equal(listTypeForText(json, '第三项'), listType, `${name} following item should stay in the list`)
    assert.deepEqual(textOrder(json), ['第一项', '第二项', '第三项'], `${name} text order must be preserved`)
    editor.destroy()
  })

  test(`${name}: Backspace at the start of an empty level-one item cancels the list`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('')])],
    })

    markFocused(editor)
    // 空段落没有文本节点：光标位置 2 即空段落内部
    editor.chain().focus().setTextSelection(2).run()
    assert.ok(isInList(editor), `${name} cursor should sit inside the empty list item`)

    pressBackspace(editor, window)

    const json = editor.getJSON()
    assert.equal(json.content[0].type, 'paragraph', `${name} empty item should cancel the list`)
    editor.destroy()
  })
}

// ---------------------------------------------------------------------------
// 多级：逐级反缩进，最后取消列表（工单描述的连续 Backspace 场景）
// ---------------------------------------------------------------------------

for (const { name, listType, makeItem } of LIST_CASES) {
  test(`${name}: repeated Backspace at line start climbs 3 -> 2 -> 1 -> plain text`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [
        makeItem('一级'),
        makeItem('二级', [
          list(listType, [makeItem('二级兄弟', [list(listType, [makeItem('三级')])])]),
        ]),
      ])],
    })

    assert.equal(visualDepthForText(editor.getJSON(), '三级'), 3, `${name} fixture should start at level three`)

    backspaceAtTextStart(editor, window, '三级')
    assert.equal(visualDepthForText(editor.getJSON(), '三级'), 2, `${name} third-level item should move to level two`)

    backspaceAtTextStart(editor, window, '三级')
    assert.equal(visualDepthForText(editor.getJSON(), '三级'), 1, `${name} item should move to level one`)

    backspaceAtTextStart(editor, window, '三级')
    const json = editor.getJSON()
    assert.equal(visualDepthForText(json, '三级'), 0, `${name} level-one item should leave the list`)
    assert.equal(listTypeForText(json, '三级'), null, `${name} exited item should not belong to a list`)
    assert.equal(listItemTypeForText(json, '三级'), null, `${name} exited item should be plain text`)
    assert.deepEqual(
      textOrder(json),
      ['一级', '二级', '二级兄弟', '三级'],
      `${name} exiting the list must preserve document order`,
    )
    editor.destroy()
  })
}

test('visual indentation also climbs down and then cancels the list', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('视觉二级', [], 1),
      listItem('视觉三级', [], 2),
    ])],
  })
  // 光标在“视觉三级”项（结构一级 + 缩进 2 级）
  assert.equal(visualDepthForText(editor.getJSON(), '视觉三级'), 3)

  backspaceAtTextStart(editor, window, '视觉三级')
  assert.equal(visualDepthForText(editor.getJSON(), '视觉三级'), 2, 'first Backspace should drop one visual level')

  backspaceAtTextStart(editor, window, '视觉三级')
  assert.equal(visualDepthForText(editor.getJSON(), '视觉三级'), 1, 'second Backspace should drop to level one')

  backspaceAtTextStart(editor, window, '视觉三级')
  const json = editor.getJSON()
  assert.equal(listTypeForText(json, '视觉三级'), null, 'third Backspace should cancel the list on the active item')
  assert.deepEqual(textOrder(json), ['一级', '视觉二级', '视觉三级'], 'text order must be preserved')
  editor.destroy()
})

test('nested item keeps its child list when outdented', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('父项', [list('bulletList', [listItem('子项')])]),
    ])],
  })

  backspaceAtTextStart(editor, window, '子项')

  const json = editor.getJSON()
  assert.equal(visualDepthForText(json, '父项'), 1, 'parent should stay at level one')
  assert.equal(visualDepthForText(json, '子项'), 1, 'child should be lifted to level one')
  assert.deepEqual(textOrder(json), ['一级', '父项', '子项'], 'lifting must preserve document order')
  editor.destroy()
})

test('cancelling a level-one item lifts its child list instead of dropping it', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('父项', [list('bulletList', [listItem('子项')])]),
    ])],
  })

  backspaceAtTextStart(editor, window, '父项')

  const json = editor.getJSON()
  assert.equal(listTypeForText(json, '父项'), null, 'parent should become plain text')
  assert.equal(listTypeForText(json, '子项'), 'bulletList', 'child list must survive the cancellation')
  assert.deepEqual(textOrder(json), ['一级', '父项', '子项'], 'child content must not be lost')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 不该接管的场景
// ---------------------------------------------------------------------------

test('Backspace in the middle of a list item is not taken over by the list handler', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [listItem('abcdef')])],
  })
  markFocused(editor)
  editor.chain().focus().setTextSelection(textEndPosition(editor, 'abcdef')).run()

  const event = pressBackspace(editor, window)

  assert.equal(event.defaultPrevented, false, 'inline Backspace must fall through to default handling')
  assert.equal(listTypeForText(editor.getJSON(), 'abcdef'), 'bulletList', 'inline Backspace must not cancel the list')
  editor.destroy()
})

test('Backspace with a selection inside a list item deletes the selection normally', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [listItem('abcdef'), listItem('保留项')])],
  })
  markFocused(editor)
  editor.chain().focus().setTextSelection({
    from: textStartPosition(editor, 'abcdef'),
    to: textEndPosition(editor, 'abcdef'),
  }).run()

  pressBackspace(editor, window)

  const json = editor.getJSON()
  assert.equal(json.content[0].type, 'bulletList', 'list wrapper should survive a selection delete')
  assert.equal(json.content[0].content.length, 2, 'list should still hold both items')
  assert.equal(listTypeForText(json, '保留项'), 'bulletList', 'the other item must stay in the list')
  assert.equal(visualDepthForText(json, '保留项'), 1, 'nesting level must not change')
  assert.deepEqual(textOrder(json), ['保留项'], 'only the selected text should be removed')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 段落行首：并入前一个列表项（删除该行），不再被还原成列表项
//
// 用户反馈：列表里从最后一项删除，删到空行后再按 Backspace 又冒出列表项。
// 根因是 ProseMirror 默认 joinBackward 会把“紧跟列表之后的段落”重新包成列表项，
// 于是“退出列表 → 又变回列表项”来回循环，这一行永远删不掉。
// ---------------------------------------------------------------------------

function emptyParagraphPosition(editor) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (found === null && node.type.name === 'paragraph' && node.content.size === 0) found = pos
    return true
  })
  if (found === null) throw new Error('empty paragraph not found')
  return found + 1
}

function countListBlocks(node) {
  let count = 0
  const walk = (n) => {
    if (!n || typeof n !== 'object') return
    if (['bulletList', 'orderedList', 'taskList'].includes(n.type)) count += 1
    if (Array.isArray(n.content)) n.content.forEach(walk)
  }
  walk(node)
  return count
}

function blockTypes(json) {
  return (json.content || []).map((node) => node.type)
}

// 文本块里的纯文本（合并后相邻文本节点会被 ProseMirror 合成一个节点，
// 所以不能再用“按整段文本查找节点”的方式断言）
function textContentOf(node) {
  let out = ''
  if (!node || typeof node !== 'object') return out
  if (node.type === 'text') out += node.text || ''
  if (Array.isArray(node.content)) node.content.forEach((child) => { out += textContentOf(child) })
  return out
}

function hasEmptyParagraph(node) {
  if (!node || typeof node !== 'object') return false
  if (node.type === 'paragraph') return textContentOf(node) === ''
  return (node.content || []).some(hasEmptyParagraph)
}

// 顶层列表块里每一项的纯文本
function listItemTexts(json, listTypeName) {
  const blocks = (json.content || []).filter((node) => node.type === listTypeName)
  return blocks.flatMap((block) => (block.content || []).map((item) => textContentOf(item)))
}

for (const { name, listType, makeItem } of LIST_CASES) {
  test(`${name}: 末项行首连续 Backspace 不会把段落还原成列表项`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('A'), makeItem('B'), makeItem('C')])],
    })

    backspaceAtTextStart(editor, window, 'C')
    let json = editor.getJSON()
    assert.deepEqual(blockTypes(json), [listType, 'paragraph'], `${name} 第一次：末项应退出列表变成段落`)

    // 再按一次：段落必须并入前一个列表项（这一行被删除），而不是重新变成列表项
    backspaceAtTextStart(editor, window, 'C')
    json = editor.getJSON()
    assert.equal(countListBlocks(json), 1, `${name} 第二次：不能新增列表块`)
    assert.deepEqual(blockTypes(json), [listType], `${name} 第二次：该行被删除，顶层只应剩一个列表块`)
    assert.deepEqual(listItemTexts(json, listType), ['A', 'BC'], `${name} 第二次：文字应并入上一项`)
    assert.equal(editor.state.selection.$from.parentOffset, 'B'.length, `${name} 第二次：光标应停在 B/C 接缝处`)
    assert.ok(!listTypeForText(json, 'C'), `${name} 第二次：不能再出现独立的 C 列表项`)
    editor.destroy()
  })

  test(`${name}: 空末项连续 Backspace 删掉整行且不再冒出列表项`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('A'), makeItem('B'), makeItem('')])],
    })

    markFocused(editor)
    editor.chain().focus().setTextSelection(emptyParagraphPosition(editor)).run()
    pressBackspace(editor, window)
    let json = editor.getJSON()
    assert.deepEqual(blockTypes(json), [listType, 'paragraph'], `${name} 第一次：空项应退出列表`)

    pressBackspace(editor, window)
    json = editor.getJSON()
    assert.deepEqual(blockTypes(json), [listType], `${name} 第二次：空行必须被删除`)
    assert.deepEqual(listItemTexts(json, listType), ['A', 'B'], `${name} 第二次：只应剩 A、B 两项`)
    assert.equal(editor.state.selection.$from.parent.textContent, 'B', `${name} 光标应回到上一项末尾`)
    editor.destroy()
  })

  test(`${name}: 列表后的普通段落行首 Backspace 并入列表最后一项`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('A'), makeItem('B')]), paragraph('尾部正文')],
    })

    backspaceAtTextStart(editor, window, '尾部正文')

    const json = editor.getJSON()
    assert.deepEqual(blockTypes(json), [listType], `${name} 段落行应消失`)
    assert.deepEqual(listItemTexts(json, listType), ['A', 'B尾部正文'], `${name} 文字应并入最后一项`)
    assert.equal(editor.state.selection.$from.parent.textContent, 'B尾部正文', `${name} 光标应落在上一项文本内`)
    assert.equal(editor.state.selection.$from.parentOffset, 'B'.length, `${name} 光标应停在 B/尾部正文 接缝处，而不是行尾`)
    editor.destroy()
  })

  test(`${name}: 空段落夹在两个列表之间，删行后合并回一个列表`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('A')]), paragraph(''), list(listType, [makeItem('C')])],
    })

    markFocused(editor)
    editor.chain().focus().setTextSelection(emptyParagraphPosition(editor)).run()
    pressBackspace(editor, window)

    const json = editor.getJSON()
    assert.equal(countListBlocks(json), 1, `${name} 相邻同类型列表必须合并回一个列表块`)
    assert.deepEqual(listItemTexts(json, listType), ['A', 'C'], `${name} 文本顺序必须保持`)
    editor.destroy()
  })
}

test('空列表项在中间：连续两次 Backspace 后整行删除且有序列表合并', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('orderedList', [listItem('A'), listItem(''), listItem('C')])],
  })

  markFocused(editor)
  editor.chain().focus().setTextSelection(emptyParagraphPosition(editor)).run()
  pressBackspace(editor, window)
  assert.equal(countListBlocks(editor.getJSON()), 2, '空项退出列表时应拆成两个列表块')

  pressBackspace(editor, window)
  const json = editor.getJSON()
  assert.equal(countListBlocks(json), 1, '删除空行后相邻有序列表必须合并回一个列表块')
  assert.deepEqual(listItemTexts(json, 'orderedList'), ['A', 'C'], '文本顺序必须保持')
  assert.equal(json.content[0].attrs.start, 1, '合并后的有序列表必须保持 start=1')
  editor.destroy()
})

test('段落跟在普通段落后仍走默认合并行为（不被本次接管影响）', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [paragraph('前一段'), paragraph('后一段')],
  })

  backspaceAtTextStart(editor, window, '后一段')

  const json = editor.getJSON()
  assert.deepEqual(blockTypes(json), ['paragraph'], '两个普通段落应合并成一个段落')
  assert.equal(textContentOf(json), '前一段后一段', '文本内容必须保持')
  editor.destroy()
})

test('列表后段落行中 Backspace 不被接管，仍按默认删除字符', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [listItem('A')]), paragraph('tail')],
  })

  markFocused(editor)
  editor.chain().focus().setTextSelection(textStartPosition(editor, 'tail') + 2).run()
  const event = pressBackspace(editor, window)

  assert.equal(event.defaultPrevented, false, '行中 Backspace 必须交回默认行为')
  const json = editor.getJSON()
  assert.deepEqual(blockTypes(json), ['bulletList', 'paragraph'], '列表块必须保持不变')
  assert.deepEqual(listItemTexts(json, 'bulletList'), ['A'], '列表项不得被改动')
  assert.equal(textContentOf(json.content[1]), 'tail', '段落不得被本次接管改动')
  editor.destroy()
})

test('段落文字并入列表项时保留 marks', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [
      list('bulletList', [listItem('A')]),
      { type: 'paragraph', content: [{ type: 'text', marks: [{ type: 'bold' }], text: '加粗正文' }] },
    ],
  })

  backspaceAtTextStart(editor, window, '加粗正文')

  const json = editor.getJSON()
  const itemParagraphs = json.content[0].content[0].content
  const merged = itemParagraphs[itemParagraphs.length - 1]
  const lastText = merged.content[merged.content.length - 1]
  assert.equal(lastText.text, '加粗正文', '文字应并入列表项最后一段')
  assert.deepEqual(lastText.marks, [{ type: 'bold' }], '并入的文字必须保留粗体 marks')
  assert.equal(editor.state.selection.$from.parentOffset, 'A'.length, '光标应停在 A/加粗正文 接缝处')
  editor.destroy()
})

test('taskList 后的段落行首 Backspace 并入待办项', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [
      { type: 'taskList', content: [taskItem('待办A'), taskItem('待办B')] },
      paragraph('尾部正文'),
    ],
  })

  backspaceAtTextStart(editor, window, '尾部正文')

  const json = editor.getJSON()
  assert.deepEqual(blockTypes(json), ['taskList'], '段落行应消失')
  assert.deepEqual(listItemTexts(json, 'taskList'), ['待办A', '待办B尾部正文'], '文字应并入待办项')
  assert.equal(editor.state.selection.$from.parentOffset, '待办B'.length, '光标应停在待办B/尾部正文接缝处')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 列表末项末尾按 Delete：与 Backspace 方向对称
//
// 用户反馈：列表最后一项按 Delete 删除后续空行时，空段落先被包成一条新的列表项
// （待办列表表现为“又冒出一条待办”），再按一次才消失。三种列表结构相同、必须一起修。
// ---------------------------------------------------------------------------

for (const { name, listType, makeItem } of LIST_CASES) {
  test(`${name}: Delete at the end of the last item removes the following empty paragraph`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('甲'), makeItem('乙')]), paragraph('')],
    })

    const event = deleteAtTextEnd(editor, window, '乙')
    const json = editor.getJSON()

    assert.equal(event.defaultPrevented, true, `${name} Delete must be taken over at the list tail`)
    assert.equal(countListBlocks(json), 1, `${name} must not create another list block`)
    assert.deepEqual(blockTypes(json), [listType], `${name} the trailing empty paragraph must be removed`)
    assert.deepEqual(listItemTexts(json, listType), ['甲', '乙'], `${name} must not create a new empty item`)
    assert.equal(editor.state.selection.$from.parent.textContent, '乙', `${name} caret should stay at the end of the last item`)
  })

  test(`${name}: Delete at the end of the last item merges the following paragraph in one step`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('甲'), makeItem('乙')]), paragraph('尾巴')],
    })

    deleteAtTextEnd(editor, window, '乙')
    const json = editor.getJSON()

    assert.deepEqual(blockTypes(json), [listType], `${name} paragraph block should disappear`)
    assert.deepEqual(listItemTexts(json, listType), ['甲', '乙尾巴'], `${name} text must merge into the current item in one step`)
    assert.equal(editor.state.selection.$from.parent.textContent, '乙尾巴', `${name} caret should follow the merged text`)
  })

  test(`${name}: Delete in the middle of an item is not taken over`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('甲'), makeItem('乙')]), paragraph('尾巴')],
    })

    markFocused(editor)
    editor.chain().focus().setTextSelection(textStartPosition(editor, '尾巴') + 1).run()
    pressDelete(editor, window)

    const json = editor.getJSON()
    assert.deepEqual(blockTypes(json), [listType, 'paragraph'], `${name} inline Delete must keep both blocks`)
    assert.deepEqual(listItemTexts(json, listType), ['甲', '乙'], `${name} list must not be changed by inline Delete`)
  })

  test(`${name}: Delete at the end of a non-last item keeps the default item join`, () => {
    const window = setupDom()
    const editor = createEditor(window, {
      type: 'doc',
      content: [list(listType, [makeItem('甲'), makeItem('乙'), makeItem('丙')])],
    })

    deleteAtTextEnd(editor, window, '乙')
    const json = editor.getJSON()

    // 非末项走 ProseMirror 默认 joinForward：后一项并入当前项的第二个段落，
    // 不生成新的列表项。这里只锁住“默认行为没有被 Delete 定制逻辑改写”。
    assert.equal(countListBlocks(json), 1, `${name} list wrapper must survive`)
    assert.deepEqual(blockTypes(json), [listType], `${name} no paragraph block should be created`)
    assert.equal(json.content[0].content.length, 2, `${name} item count must stay unchanged`)
    const joinedItem = json.content[0].content[1]
    assert.equal(joinedItem.content.length, 2, `${name} next item should join as a second paragraph`)
    assert.equal(textContentOf(joinedItem.content[0]), '乙', `${name} first paragraph must stay`)
    assert.equal(textContentOf(joinedItem.content[1]), '丙', `${name} joined paragraph text must be kept`)
  })
}

test('Delete merge into a task item keeps the following paragraph marks', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [
      { type: 'taskList', content: [taskItem('待办')] },
      { type: 'paragraph', content: [{ type: 'text', marks: [{ type: 'bold' }], text: '加粗尾巴' }] },
    ],
  })

  deleteAtTextEnd(editor, window, '待办')

  const json = editor.getJSON()
  assert.deepEqual(blockTypes(json), ['taskList'], 'the following paragraph block should disappear')
  const texts = json.content[0].content[0].content[0].content
  const merged = texts.find((node) => node.text === '加粗尾巴')
  assert.ok(merged, 'merged text must be kept')
  assert.deepEqual(merged.marks, [{ type: 'bold' }], 'merged text must keep marks')
})

// ---------------------------------------------------------------------------
// 空列表项前面是“带嵌套子列表的列表项”时，删行不再横跳
//
// 用户反馈：图 5 结构下光标停在空的一级项“2.”，Backspace 删掉序号后再按，
// 空行又被包回列表、序号重新出现，在同一行反复横跳。
// 根因是 joinParagraphBackwardIntoPreviousList 只认上一项的 lastChild 是文本块；
// 上一项末尾挂着嵌套子列表时 lastChild 是 list，于是误判为无法并入并回退到
// core joinBackward，被重新包成列表项。
// ---------------------------------------------------------------------------

test('空项前面是带嵌套子列表的列表项：连续 Backspace 删行后不再变回列表项', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('orderedList', [
      listItem('飒飒', [
        list('orderedList', [
          listItem('打'),
          listItem('地方法', [list('orderedList', [listItem('打')])]),
        ]),
      ]),
      listItem(''),
      listItem('撒啊'),
      listItem('ss'),
      listItem('发给过'),
    ])],
  })

  markFocused(editor)
  editor.chain().focus().setTextSelection(emptyParagraphPosition(editor)).run()
  pressBackspace(editor, window)

  let json = editor.getJSON()
  assert.deepEqual(blockTypes(json), ['orderedList', 'paragraph', 'orderedList'],
    '第一次 Backspace：空项退出列表，前后拆成两个列表块')

  // 第二次：空行必须被删除、前后两个列表合并回一个，不能再出现“空行 ↔ 列表项”横跳
  markFocused(editor)
  editor.chain().focus().setTextSelection(emptyParagraphPosition(editor)).run()
  pressBackspace(editor, window)
  json = editor.getJSON()

  assert.deepEqual(blockTypes(json), ['orderedList'], '空行删除后顶层必须合并回一个列表块')
  assert.equal(json.content.length, 1, '顶层不能再出现被重新包成列表项的第三块')
  assert.deepEqual(
    json.content[0].content.map((item) => item.content[0].content[0].text),
    ['飒飒', '撒啊', 'ss', '发给过'],
    '删行后外层项数必须回到 4 且顺序不变',
  )

  // 嵌套子列表必须完整保留，不能被并入过程丢掉
  const nested = json.content[0].content[0].content.slice(1)
  assert.deepEqual(nested.map((node) => node.type), ['orderedList'], '嵌套子列表必须保留')
  assert.deepEqual(
    nested[0].content.map((item) => item.content[0].content[0].text),
    ['打', '地方法'],
  )
  assert.deepEqual(
    nested[0].content[1].content[1].content.map((item) => item.content[0].content[0].text),
    ['打'],
    '三级子列表内容不能丢失',
  )

  // 再按一次：无论光标落在上一项末尾还是下一项行首，都不能再出现空行/空项横跳。
  pressBackspace(editor, window)
  const afterThird = editor.getJSON()
  assert.equal(hasEmptyParagraph(afterThird), false, '第三次不能再冒出空行（横跳根因已消除）')
  editor.destroy()
})

// ---------------------------------------------------------------------------
// 范围选区反缩进：选中的多个同级项必须一起回退，而不是只回退光标那一项
// ---------------------------------------------------------------------------

function selectFromTo(editor, fromText, toText) {
  editor.chain().focus().setTextSelection({
    from: textStartPosition(editor, fromText),
    to: textEndPosition(editor, toText),
  }).run()
}

test('range selection: Shift+Tab outdents every selected visual-indent item', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('甲项', [], 1),
      listItem('乙项', [], 1),
      listItem('丙项', [], 1),
    ])],
  })

  selectFromTo(editor, '甲项', '丙项')
  assert.equal(liftActiveListItem(editor), true, 'Shift+Tab should be handled')

  const json = editor.getJSON()
  for (const text of ['甲项', '乙项', '丙项']) {
    assert.equal(visualDepthForText(json, text), 1, `${text} 应回退到一级`)
  }
  assert.deepEqual(textOrder(json), ['一级', '甲项', '乙项', '丙项'], '文本顺序必须保持不变')
  assert.equal(editor.state.selection.empty, false, '反缩进后选区应保持')
  editor.destroy()
})

test('range selection: only the selected items are outdented, siblings stay put', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('甲项', [], 1),
      listItem('未选中乙', [], 1),
      listItem('丙项', [], 1),
    ])],
  })

  selectFromTo(editor, '甲项', '甲项')
  assert.equal(liftActiveListItem(editor), true)

  const json = editor.getJSON()
  assert.equal(visualDepthForText(json, '甲项'), 1, '选中项应回退到一级')
  assert.equal(visualDepthForText(json, '未选中乙'), 2, '未选中项必须保持二级')
  assert.equal(visualDepthForText(json, '丙项'), 2, '未选中项必须保持二级')
  editor.destroy()
})

// 方案 A（2026-10-16 确认）：范围选区里一级项与单光标同规则 —— 这一步就退出列表。
// 视觉额外缩进项（同级 + dvnIndentLevel）各降一级，仍是列表项。
test('range selection: level-one item exits the list while visual-indent items outdent', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('甲项'),
      listItem('乙项', [], 1),
      listItem('丙项', [], 1),
    ])],
  })

  selectFromTo(editor, '甲项', '丙项')
  assert.equal(liftActiveListItem(editor), true)

  const json = editor.getJSON()
  // 甲项 是一级项：按 A 直接退出列表，变成普通段落
  assert.equal(listTypeForText(json, '甲项'), null, '一级项应退出列表')
  assert.equal(listItemTypeForText(json, '甲项'), null, '一级项应变成普通段落')
  assert.equal(visualDepthForText(json, '甲项'), 0, '退出列表后不再有层级')
  // 视觉额外缩进项各降一级，仍留在列表里
  assert.equal(visualDepthForText(json, '乙项'), 1, '乙项 应降到一级')
  assert.equal(visualDepthForText(json, '丙项'), 1, '丙项 应降到一级')
  assert.deepEqual(textOrder(json), ['甲项', '乙项', '丙项'], '文本顺序必须保持不变')
  editor.destroy()
})

test('range selection: structurally nested items still lift through the default path', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('父项', [
        list('bulletList', [listItem('子一'), listItem('子二')]),
      ]),
    ])],
  })

  selectFromTo(editor, '子一', '子二')
  assert.equal(liftActiveListItem(editor), true)

  const json = editor.getJSON()
  assert.equal(visualDepthForText(json, '子一'), 1, '子一 应提升到一级')
  assert.equal(visualDepthForText(json, '子二'), 1, '子二 应提升到一级')
  assert.deepEqual(
    textOrder(json),
    ['一级', '父项', '子一', '子二'],
    '结构提升必须保持文本顺序',
  )
  editor.destroy()
})

test('range selection: Tab indents every selected visual-indent item', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('甲项', [], 1),
      listItem('乙项', [], 1),
      listItem('丙项', [], 1),
    ])],
  })

  selectFromTo(editor, '甲项', '丙项')
  assert.equal(sinkActiveListItem(editor), true, 'Tab should be handled')

  const json = editor.getJSON()
  for (const text of ['甲项', '乙项', '丙项']) {
    assert.equal(visualDepthForText(json, text), 3, `${text} 应升到三级`)
  }
  assert.deepEqual(textOrder(json), ['一级', '甲项', '乙项', '丙项'], '文本顺序必须保持不变')
  editor.destroy()
})

test('range selection: structurally nested items still sink through the default path', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('甲项'),
      listItem('乙项'),
    ])],
  })

  selectFromTo(editor, '甲项', '乙项')
  assert.equal(sinkActiveListItem(editor), true)

  const json = editor.getJSON()
  assert.equal(visualDepthForText(json, '甲项'), 2, '结构下沉应由 ProseMirror 处理')
  assert.deepEqual(textOrder(json), ['一级', '甲项', '乙项'], '文本顺序必须保持不变')
  editor.destroy()
})

test('range selection: mixed list types lift every selected item, not just the first', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('一级'),
      listItem('父项', [
        list('taskList', [taskItem('甲项'), taskItem('乙项'), taskItem('丙项')]),
      ]),
    ])],
  })

  selectFromTo(editor, '甲项', '丙项')
  assert.equal(liftActiveListItem(editor), true)

  const json = editor.getJSON()
  for (const text of ['甲项', '乙项', '丙项']) {
    assert.equal(visualDepthForText(json, text), 1, `${text} 应提升到一级`)
    assert.equal(listItemTypeForText(json, text), 'taskItem', `${text} 应保持待办项类型`)
  }
  assert.deepEqual(
    textOrder(json),
    ['一级', '父项', '甲项', '乙项', '丙项'],
    '提升后文本顺序必须保持不变',
  )
  editor.destroy()
})

test('range selection: mixed list types keep unselected siblings inside the parent item', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('父项', [
        list('taskList', [taskItem('甲项'), taskItem('未选中'), taskItem('丙项')]),
      ]),
    ])],
  })

  selectFromTo(editor, '甲项', '甲项')
  assert.equal(liftActiveListItem(editor), true)

  const json = editor.getJSON()
  assert.equal(visualDepthForText(json, '甲项'), 1, '选中项提升到一级')
  assert.deepEqual(
    textOrder(json),
    ['父项', '甲项', '未选中', '丙项'],
    '未选中兄弟必须保留在父项下，且顺序不变',
  )
  editor.destroy()
})

test('range selection: repeated Shift+Tab never re-indents unselected siblings', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('甲一'),
      listItem('乙二', [
        list('bulletList', [listItem('新一', [], 1), listItem('新二', [], 1)]),
      ]),
      listItem('丙三'),
      listItem('丁四'),
      listItem('戊五'),
    ])],
  })
  // 前提：新一/新二 结构二级 + 视觉额外缩进 1 => 视觉三级
  assert.equal(visualDepthForText(editor.getJSON(), '新一'), 3)

  selectFromTo(editor, '乙二', '新二')
  assert.equal(liftActiveListItem(editor), true)

  const json = editor.getJSON()
  // 方案 A：选区里的一级项与单光标同规则，这一步就退出列表。
  assert.equal(listTypeForText(json, '乙二'), null, '一级项在范围选区下应退出列表')
  assert.equal(listItemTypeForText(json, '乙二'), null, '一级项应变成普通段落')
  // 选中的子项各降一级
  assert.equal(visualDepthForText(json, '新一'), 2, '新一 属于选中项，应降一级')
  assert.equal(visualDepthForText(json, '新二'), 2, '新二 属于选中项，应降一级')
  // 关键回归点：未选中的兄弟项绝不能被改缩进
  for (const text of ['丙三', '丁四', '戊五']) {
    assert.equal(visualDepthForText(json, text), 1, `${text} 是未选中项，必须保持一级`)
  }
  assert.deepEqual(
    textOrder(json),
    ['甲一', '乙二', '新一', '新二', '丙三', '丁四', '戊五'],
    '文本顺序必须保持不变',
  )
  // 选区必须仍完整覆盖原来这三项，不能缩水
  assert.equal(
    editor.state.doc.textBetween(editor.state.selection.from, editor.state.selection.to, '\n'),
    '乙二\n新一\n新二',
    '反缩进后选区应完整覆盖选中的三项',
  )
  editor.destroy()
})

test('range selection: same visual level with mixed internal representation outdents all selected items', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      // 甲一 下面挂着结构二级的“大萨达撒”
      listItem('甲一', [list('bulletList', [listItem('大萨达撒')])]),
      // 另外两项是“结构一级 + 视觉额外缩进 1”，视觉上同样是二级
      listItem('大萨达', [], 1),
      listItem('大萨达', [], 1),
      listItem('末尾'),
    ])],
  })
  assert.equal(visualDepthForText(editor.getJSON(), '大萨达撒'), 2)
  assert.equal(visualDepthForText(editor.getJSON(), '大萨达'), 2)

  // 选区从结构二级项拖到最后一个视觉二级项（同一视觉层级、两种内部表示）
  editor.chain().focus().setTextSelection({
    from: textStartPosition(editor, '大萨达撒'),
    to: lastTextEndPosition(editor, '大萨达'),
  }).run()
  assert.equal(liftActiveListItem(editor), true)

  const json = editor.getJSON()
  assert.equal(visualDepthForText(json, '大萨达撒'), 1, '结构二级项应降到一级')
  // 两个“大萨达”文本相同，必须逐项检查，不能只看第一次出现
  assert.deepEqual(visualDepthsForAll(json, '大萨达'), [1, 1], '两个选中项都应降到一级')
  // 未选中项不能被牵动
  assert.equal(visualDepthForText(json, '甲一'), 1, '甲一 是祖先容器，必须保持一级')
  assert.equal(visualDepthForText(json, '末尾'), 1, '末尾 未选中，必须保持一级')
  assert.deepEqual(
    textOrder(json),
    ['甲一', '大萨达撒', '大萨达', '大萨达', '末尾'],
    '文本顺序必须保持不变',
  )
  editor.destroy()
})

test('range selection: mixed internal representation indents every selected item', () => {
  const window = setupDom()
  const editor = createEditor(window, {
    type: 'doc',
    content: [list('bulletList', [
      listItem('甲一', [list('bulletList', [listItem('大萨达撒')])]),
      listItem('大萨达', [], 1),
      listItem('大萨达', [], 1),
      listItem('末尾'),
    ])],
  })

  editor.chain().focus().setTextSelection({
    from: textStartPosition(editor, '大萨达撒'),
    to: lastTextEndPosition(editor, '大萨达'),
  }).run()
  assert.equal(sinkActiveListItem(editor), true)

  const json = editor.getJSON()
  assert.equal(visualDepthForText(json, '大萨达撒'), 3, '结构二级项应升到三级')
  assert.deepEqual(visualDepthsForAll(json, '大萨达'), [3, 3], '两个视觉二级项都应升到三级')
  assert.equal(visualDepthForText(json, '甲一'), 1, '甲一 未选中，必须保持一级')
  assert.equal(visualDepthForText(json, '末尾'), 1, '末尾 未选中，必须保持一级')
  editor.destroy()
})
