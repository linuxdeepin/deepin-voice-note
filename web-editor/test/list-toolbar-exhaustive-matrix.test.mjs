// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 列表工具栏全组合矩阵。重点防止“修一个入口，另一个入口又坏”：
// - 3 种列表类型 × 3 层叶子位置 × 3 个光标位置
// - 3 种类型 × 3 层选择位置 × 2 种选区
// - 3 种源类型 × 3 种目标类型的多项转换
// - 27 种三连按钮目标序列
// - 按钮与 Tab/Shift+Tab 交叉
// 所有组合统一校验：文本顺序、未选中项类型/层级/勾选/序号不变，
// 选中项只出现目标类型或完全退出列表，绝不产生额外一层列表。

import assert from 'node:assert/strict'
import test from 'node:test'
import { Window } from 'happy-dom'
import { Editor } from '@tiptap/core'

import { createTiptapExtensions } from '../src/runtime/tiptap-extensions.js'
import { switchSelectedListType, toggleActiveListFormatting } from '../src/runtime/list-behavior.js'

const LIST_TYPES = ['bulletList', 'orderedList', 'taskList']
const ITEM_TYPES = {
  bulletList: 'listItem',
  orderedList: 'listItem',
  taskList: 'taskItem',
}

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

function item(type, text, children = [], attrs = {}) {
  return {
    type: ITEM_TYPES[type],
    attrs: {
      dvnIndentLevel: 0,
      ...(type === 'taskList' ? { checked: text === 'B' || text === 'D' } : {}),
      ...attrs,
    },
    content: [paragraph(text), ...children],
  }
}

function list(type, content, start = 1) {
  return {
    type,
    attrs: type === 'orderedList' ? { start } : {},
    content,
  }
}

function createEditor(content) {
  const window = setupDom()
  const element = window.document.createElement('div')
  window.document.body.appendChild(element)
  const editor = new Editor({ element, extensions: createTiptapExtensions(), content })
  return { editor, window }
}

function findText(editor, text) {
  let found = null
  editor.state.doc.descendants((node, pos) => {
    if (found != null) return false
    if (node.isText && node.text === text) {
      found = { from: pos, to: pos + node.nodeSize }
      return false
    }
    return true
  })
  assert.ok(found, `text not found: ${text}`)
  return found
}

function setCursor(editor, text, offset) {
  const range = findText(editor, text)
  editor.commands.setTextSelection(range.from + offset)
}

function selectText(editor, startText, startOffset, endText, endOffset) {
  const start = findText(editor, startText)
  const end = findText(editor, endText)
  editor.commands.setTextSelection({ from: start.from + startOffset, to: end.from + endOffset })
}

function selectedText(editor) {
  const { from, to } = editor.state.selection
  return editor.state.doc.textBetween(from, to, '|')
}

function textOrder(doc) {
  const order = []
  const walk = (node) => {
    if (node.isTextblock && !node.type.name.startsWith('list')) {
      if (node.textContent) order.push(node.textContent)
      return
    }
    node.forEach((child) => walk(child))
  }
  walk(doc)
  return order
}

function itemSnapshot(doc) {
  const result = []
  const walkDescendants = (node, listDepth = 0) => {
    node.forEach((child) => {
      if (LIST_TYPES.includes(child.type.name)) {
        walkList(child, listDepth)
      } else {
        walkDescendants(child, listDepth)
      }
    })
  }
  const walkList = (listNode, listDepth) => {
    const start = listNode.type.name === 'orderedList'
      ? Number(listNode.attrs?.start ?? 1)
      : null
    listNode.forEach((listItem, _offset, index) => {
      const own = listItem.firstChild?.textContent ?? ''
      const extraIndent = Number(listItem.attrs?.dvnIndentLevel) || 0
      result.push({
        text: own,
        itemType: listItem.type.name,
        listType: listNode.type.name,
        // 视觉层级 = 结构层级 + dvnIndentLevel。取消嵌套项后允许结构提升，
        // 但必须用额外缩进补回视觉层级，不能让用户看到内容跳层。
        depth: Math.min(3, listDepth + Math.max(0, Math.floor(extraIndent))),
        checked: listNode.type.name === 'taskList' ? Boolean(listItem.attrs?.checked) : null,
        number: start == null ? null : start + index,
      })
      walkDescendants(listItem, listDepth + 1)
    })
  }
  walkDescendants(doc)
  return result
}

function withoutNumbers(entries) {
  return entries.map(({ number: _number, ...entry }) => entry)
}

function assertOrderedFragmentsStartAtOne(doc) {
  const starts = []
  doc.descendants((node) => {
    if (node.type.name === 'orderedList') starts.push(Number(node.attrs?.start ?? 1))
    return true
  })
  assert.deepEqual(starts, starts.map(() => 1), '每个被拆分出的有序片段都应从 1 重新编号')
}

function contextForText(editor, text) {
  const range = findText(editor, text)
  const $pos = editor.state.doc.resolve(range.from)
  const listTypes = []
  const itemTypes = []
  for (let depth = 1; depth <= $pos.depth; depth += 1) {
    const node = $pos.node(depth)
    if (LIST_TYPES.includes(node.type.name)) listTypes.push(node.type.name)
    if (node.type.name === 'listItem' || node.type.name === 'taskItem') itemTypes.push(node.type.name)
  }
  return { listTypes, itemTypes }
}

function pressTab(editor, window, shiftKey = false) {
  editor.isFocused = true
  const event = new window.KeyboardEvent('keydown', {
    key: 'Tab',
    shiftKey,
    bubbles: true,
    cancelable: true,
  })
  editor.view.dom.dispatchEvent(event)
  return event
}

function leafDepthDoc(type, checked = null) {
  const attrs = type === 'taskList' && checked != null ? { checked } : {}
  const c = item(type, 'C', [], attrs)
  const b = item(type, 'B', [list(type, [c], 6)], attrs)
  const d = item(type, 'D', [], attrs)
  const a = item(type, 'A', [list(type, [b, d], 5)], attrs)
  const e = item(type, 'E', [], attrs)
  return { type: 'doc', content: [list(type, [a, e], 4)] }
}

const TARGETS = ['C', 'D', 'E']

function expectedUnselected(before, targetText) {
  return before.filter((entry) => entry.text !== targetText)
}

test('折叠光标 × 3 类型 × 3 层叶子 × 3 光标位置：只取消目标项并可无损重应用', () => {
  const failures = []
  for (const type of LIST_TYPES) {
    const checkedVariants = type === 'taskList' ? [false, true] : [null]
    for (const checked of checkedVariants) {
      for (const target of TARGETS) {
        for (const offsetName of ['start', 'middle', 'end']) {
          const { editor } = createEditor(leafDepthDoc(type, checked))
          const before = itemSnapshot(editor.state.doc)
          const targetRange = findText(editor, target)
          const offset = offsetName === 'start'
            ? 0
            : offsetName === 'middle'
              ? Math.floor(targetRange.to - targetRange.from)
              : targetRange.to - targetRange.from
          const stateLabel = checked == null ? '' : `/${checked ? 'checked' : 'unchecked'}`
          const label = `${type}${stateLabel}/${target}/${offsetName}`
          setCursor(editor, target, offset)
          const untouched = expectedUnselected(before, target)

          try {
            assert.equal(toggleActiveListFormatting(editor, type), true, `${label}: 首次取消未处理`)
            assert.deepEqual(textOrder(editor.state.doc), before.map((entry) => entry.text), `${label}: 取消后文本顺序变化`)
            assert.deepEqual(withoutNumbers(itemSnapshot(editor.state.doc)), withoutNumbers(untouched), `${label}: 未选中项类型/层级/勾选变化`)
            assert.deepEqual(contextForText(editor, target), { listTypes: [], itemTypes: [] }, `${label}: 取消后被点项仍在列表里`)
            assert.equal(
              editor.state.selection.$from.parent.attrs.dvnIndentLevel,
              before.find((entry) => entry.text === target).depth,
              `${label}: 取消后被点项丢失视觉缩进`,
            )
            assert.equal(editor.state.selection.empty, true, `${label}: 取消后光标丢失`)
            assert.equal(editor.state.selection.$from.parent.textContent, target, `${label}: 光标落在错误文本块`)
            assert.equal(editor.state.selection.$from.parentOffset, offset, `${label}: 取消后光标偏移漂移`)

            assert.equal(toggleActiveListFormatting(editor, type), true, `${label}: 重应用未处理`)
            assert.deepEqual(textOrder(editor.state.doc), before.map((entry) => entry.text), `${label}: 重应用后文本顺序变化`)
            assert.deepEqual(withoutNumbers(itemSnapshot(editor.state.doc).filter((entry) => entry.text !== target)), withoutNumbers(untouched), `${label}: 重应用改变了未选中项`)
            assert.deepEqual(contextForText(editor, target), { listTypes: [type], itemTypes: [ITEM_TYPES[type]] }, `${label}: 重应用出现多余层级`)
            assert.equal(editor.state.selection.$from.parent.textContent, target, `${label}: 重应用后光标落错`)
            assert.equal(editor.state.selection.$from.parentOffset, offset, `${label}: 重应用后光标偏移漂移`)

            assert.equal(toggleActiveListFormatting(editor, type), true, `${label}: 二次取消未处理`)
            assert.deepEqual(withoutNumbers(itemSnapshot(editor.state.doc)), withoutNumbers(untouched), `${label}: 二次取消改变了未选中项`)
            assert.deepEqual(contextForText(editor, target), { listTypes: [], itemTypes: [] }, `${label}: 二次取消后仍有列表层级`)
          } catch (error) {
            failures.push(error.message)
          }
          editor.destroy()
        }
      }
    }
  }
  assert.deepEqual(failures, [], `折叠光标矩阵失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

test('截图场景：三层混合列表中取消中间无序项，文字保持原视觉缩进且可无损重应用', () => {
  const { editor } = createEditor({
    type: 'doc',
    content: [list('orderedList', [
      item('orderedList', '一级', [list('orderedList', [
        item('orderedList', '二级', [
          list('bulletList', [item('bulletList', '目标')]),
          list('taskList', [item('taskList', '待办', [], { checked: false })]),
        ]),
      ])]),
      item('orderedList', '尾部'),
    ], 1)],
  })
  const before = itemSnapshot(editor.state.doc)
  setCursor(editor, '目标', 2)
  const targetBefore = before.find((entry) => entry.text === '目标')

  assert.equal(toggleActiveListFormatting(editor, 'bulletList'), true)
  assert.deepEqual(contextForText(editor, '目标'), { listTypes: [], itemTypes: [] })
  assert.equal(editor.state.selection.$from.parent.attrs.dvnIndentLevel, targetBefore.depth)
  assert.equal(editor.state.selection.$from.parentOffset, 2)
  assert.deepEqual(withoutNumbers(itemSnapshot(editor.state.doc)), withoutNumbers(expectedUnselected(before, '目标')))
  assertOrderedFragmentsStartAtOne(editor.state.doc)

  assert.equal(toggleActiveListFormatting(editor, 'bulletList'), true)
  const targetAfter = itemSnapshot(editor.state.doc).find((entry) => entry.text === '目标')
  assert.equal(targetAfter.depth, targetBefore.depth)
  assert.equal(targetAfter.listType, 'bulletList')
  assert.equal(targetAfter.itemType, 'listItem')
  const targetTextblock = editor.state.selection.$from.parent
  assert.equal(targetTextblock.attrs.dvnIndentLevel, 0, '缩进应转移到 listItem，不能在段落上重复叠加')
  assert.deepEqual(
    withoutNumbers(itemSnapshot(editor.state.doc).filter((entry) => entry.text !== '目标')),
    withoutNumbers(expectedUnselected(before, '目标')),
  )
  assertOrderedFragmentsStartAtOne(editor.state.doc)
  editor.destroy()
})

test('截图场景：扁平视觉层级整体转有序后，序号按可见节点分组重置', () => {
  const { editor } = createEditor({
    type: 'doc',
    content: [list('bulletList', [
      item('bulletList', 'A', [], { dvnIndentLevel: 0 }),
      item('bulletList', 'B', [], { dvnIndentLevel: 1 }),
      item('bulletList', 'C', [], { dvnIndentLevel: 2 }),
      item('bulletList', 'D', [], { dvnIndentLevel: 2 }),
      item('bulletList', 'E', [
        list('bulletList', [item('bulletList', 'F')]),
        list('taskList', [item('taskList', '大萨达')]),
      ], { dvnIndentLevel: 0 }),
    ])],
  })

  editor.commands.selectAll()
  assert.equal(switchSelectedListType(editor, 'orderedList'), true)

  const root = editor.state.doc.firstChild
  assert.equal(root.type.name, 'orderedList')
  assert.deepEqual(
    Array.from({ length: root.childCount }, (_, index) => root.child(index).firstChild.textContent),
    ['A', 'B', 'C', 'D', 'E'],
  )
  assert.deepEqual(
    Array.from(editor.view.dom.querySelectorAll('ol > li'))
      .map((item) => Number(/--dvn-list-number: "(\d+)"/.exec(item.getAttribute('style'))?.[1])),
    [1, 1, 1, 2, 2, 1, 2],
    '根组应为 1,1,1,2,2，E 的子组应为 1,2；不能把整个扁平列表编成 1~7',
  )
  editor.destroy()
})

test('混合待办/普通段落/深缩进无序全选整体转换：后续根项编号必须连续', () => {
  for (const targetType of LIST_TYPES) {
    const { editor } = createEditor({
      type: 'doc',
      content: [
        list('taskList', [item('taskList', '前置项'), item('taskList', '父级')]),
        paragraph('目标项'),
        list('bulletList', [item('bulletList', '目标孙项', [], { dvnIndentLevel: 2 })]),
        list('taskList', [item('taskList', '后续项')]),
      ],
    })

    editor.commands.selectAll()
    const selectedBefore = selectedText(editor)
    assert.equal(switchSelectedListType(editor, targetType), true, `${targetType}: 整体转换失败`)
    assert.equal(editor.state.doc.childCount, 1, `${targetType}: 顶层列表被拆成多个块`)
    assert.equal(editor.state.doc.firstChild.type.name, targetType, `${targetType}: 顶层列表类型错误`)
    assert.deepEqual(
      itemSnapshot(editor.state.doc).map((entry) => entry.text),
      ['前置项', '父级', '目标项', '目标孙项', '后续项'],
      `${targetType}: 文本顺序变化`,
    )
    assert.deepEqual(
      itemSnapshot(editor.state.doc).map((entry) => entry.depth),
      [0, 0, 0, 2, 0],
      `${targetType}: 深层孙项或后续根项层级错误`,
    )
    assert.equal(selectedText(editor), selectedBefore, `${targetType}: 整体转换后选区漂移`)

    if (targetType === 'orderedList') {
      assert.deepEqual(
        Array.from(editor.view.dom.querySelectorAll('ol > li'))
          .map((itemNode) => Number(/--dvn-list-number: "(\d+)"/.exec(itemNode.getAttribute('style'))?.[1])),
        [1, 2, 3, 1, 4],
        '目标孙项应在目标项下从 1 编号，后续根项应继续编号 4',
      )
    }
    editor.destroy()
  }
})

test('有序列表首项/中间项/末项切换类型后，剩余片段全部从 1 重新编号', () => {
  const texts = ['A', 'B', 'C', 'D']
  const expectedNumbers = [
    [1, 2, 3],
    [1, 1, 2],
    [1, 2, 1],
    [1, 2, 3],
  ]
  for (const targetType of ['bulletList', 'taskList']) {
    for (let targetIndex = 0; targetIndex < texts.length; targetIndex += 1) {
      const { editor } = createEditor({
        type: 'doc',
        content: [list('orderedList', texts.map((text) => item('orderedList', text)), 10)],
      })
      const label = `${targetType}/切换第${targetIndex + 1}项`

      setCursor(editor, texts[targetIndex], 1)
      assert.equal(toggleActiveListFormatting(editor, targetType), true, `${label}: 切换类型失败`)
      assert.deepEqual(textOrder(editor.state.doc), texts, `${label}: 文本顺序变化`)
      assert.deepEqual(
        itemSnapshot(editor.state.doc).map((entry) => entry.number).filter((number) => number != null),
        expectedNumbers[targetIndex],
        `${label}: 剩余有序片段没有从 1 重新编号`,
      )
      assert.deepEqual(contextForText(editor, texts[targetIndex]), {
        listTypes: [targetType],
        itemTypes: [ITEM_TYPES[targetType]],
      }, `${label}: 目标项列表类型错误`)
      assertOrderedFragmentsStartAtOne(editor.state.doc)

      assert.equal(toggleActiveListFormatting(editor, 'orderedList'), true, `${label}: 恢复有序失败`)
      const root = editor.state.doc.firstChild
      assert.equal(editor.state.doc.childCount, 1, `${label}: 相邻有序片段未合并`)
      assert.equal(root.type.name, 'orderedList', `${label}: 恢复后不是有序列表`)
      assert.deepEqual(
        Array.from({ length: root.childCount }, (_, index) => root.child(index).firstChild.textContent),
        texts,
        `${label}: 恢复后文本顺序错误`,
      )
      assert.deepEqual(
        itemSnapshot(editor.state.doc).map((entry) => entry.number),
        [1, 2, 3, 4],
        `${label}: 恢复后序号没有连续重排`,
      )
      editor.destroy()
    }
  }
})

test('显式选区 × 3 类型 × 3 层叶子 × 全文/局部：只取消选中项并保持未选中层级', () => {
  const failures = []
  for (const type of LIST_TYPES) {
    const checkedVariants = type === 'taskList' ? [false, true] : [null]
    for (const checked of checkedVariants) {
      for (const target of TARGETS) {
        for (const mode of ['full', 'partial']) {
          const { editor } = createEditor(leafDepthDoc(type, checked))
          const before = itemSnapshot(editor.state.doc)
          const range = findText(editor, target)
          const from = mode === 'full' ? range.from : range.from + 1
          const to = mode === 'full' ? range.to : range.to - 1
          editor.commands.setTextSelection({ from, to })
          const selectedBefore = selectedText(editor)
          const untouched = expectedUnselected(before, target)
          const stateLabel = checked == null ? '' : `/${checked ? 'checked' : 'unchecked'}`
          const label = `${type}${stateLabel}/${target}/${mode}`

          try {
            assert.equal(switchSelectedListType(editor, type), true, `${label}: 取消失败`)
            assert.deepEqual(textOrder(editor.state.doc), before.map((entry) => entry.text), `${label}: 文本顺序变化`)
            assert.deepEqual(withoutNumbers(itemSnapshot(editor.state.doc)), withoutNumbers(untouched), `${label}: 未选中项变化`)
            assert.deepEqual(contextForText(editor, target), { listTypes: [], itemTypes: [] }, `${label}: 取消后仍在列表`)
            assert.equal(editor.state.selection.empty, false, `${label}: 选区消失`)
            assert.equal(selectedText(editor), selectedBefore, `${label}: 选区文本漂移`)

            const nextType = LIST_TYPES[(LIST_TYPES.indexOf(type) + 1) % LIST_TYPES.length]
            assert.equal(switchSelectedListType(editor, nextType), true, `${label}: 取消后重应用失败`)
            assert.deepEqual(withoutNumbers(itemSnapshot(editor.state.doc).filter((entry) => entry.text !== target)), withoutNumbers(untouched), `${label}: 重应用改变未选中项`)
            assert.deepEqual(contextForText(editor, target), {
              listTypes: [nextType],
              itemTypes: [ITEM_TYPES[nextType]],
            }, `${label}: 重应用出现多余层级`)
            assert.equal(selectedText(editor), selectedBefore, `${label}: 重应用后选区漂移`)
          } catch (error) {
            failures.push(error.message)
          }
          editor.destroy()
        }
      }
    }
  }
  assert.deepEqual(failures, [], `显式选区矩阵失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

test('同级多选 × 3 源类型 × 3 目标类型：中间段转换与取消都不影响前后项', () => {
  const failures = []
  const texts = ['P0', 'P1', 'P2', 'P3', 'P4', 'P5', 'P6']
  for (const sourceType of LIST_TYPES) {
    for (const targetType of LIST_TYPES) {
      const { editor } = createEditor({
        type: 'doc',
        content: [list(sourceType, texts.map((text) => item(sourceType, text)), sourceType === 'orderedList' ? 10 : 1)],
      })
      const beforeItems = itemSnapshot(editor.state.doc)
      const untouchedBefore = beforeItems.filter((entry) => !['P2', 'P3', 'P4'].includes(entry.text))
      selectText(editor, 'P2', 0, 'P4', 2)
      const selectedBefore = selectedText(editor)
      const label = `${sourceType}->${targetType}`

      try {
        let expected = sourceType
        for (const [clickNumber, ordinal] of ['首次', '第二次', '第三次'].entries()) {
          expected = expected === targetType ? null : targetType
          assert.equal(switchSelectedListType(editor, targetType), true, `${label}: ${ordinal}点击失败`)
          assert.deepEqual(textOrder(editor.state.doc), texts, `${label}: ${ordinal}点击文本顺序变化`)
          assert.deepEqual(
            withoutNumbers(itemSnapshot(editor.state.doc).filter((entry) => !['P2', 'P3', 'P4'].includes(entry.text))),
            withoutNumbers(untouchedBefore),
            `${label}: ${ordinal}点击影响前后未选中项`,
          )
          for (const text of ['P2', 'P3', 'P4']) {
            assert.deepEqual(contextForText(editor, text), expected == null
              ? { listTypes: [], itemTypes: [] }
              : { listTypes: [targetType], itemTypes: [ITEM_TYPES[targetType]] },
            `${label}: ${text} ${ordinal}点击状态错误，期望 ${expected}`)
          }
          assert.equal(selectedText(editor), selectedBefore, `${label}: ${ordinal}点击选区漂移`)
        }
      } catch (error) {
        failures.push(error.message)
      }
      editor.destroy()
    }
  }
  assert.deepEqual(failures, [], `同级多选矩阵失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

test('三连按钮目标序列 3^3：每一步状态、文本和选区都必须可预测', () => {
  const failures = []
  const texts = ['P0', 'P1', 'P2', 'P3', 'P4']
  for (const sourceType of LIST_TYPES) {
    for (const first of LIST_TYPES) {
      for (const second of LIST_TYPES) {
        for (const third of LIST_TYPES) {
          const { editor } = createEditor({
            type: 'doc',
            content: [list(sourceType, texts.map((text) => item(sourceType, text)))],
          })
          selectText(editor, 'P1', 0, 'P3', 2)
          const selectedBefore = selectedText(editor)
          let expected = sourceType
          const label = `${sourceType}:[${first},${second},${third}]`
          try {
            for (const target of [first, second, third]) {
              expected = expected === target ? null : target
              assert.equal(switchSelectedListType(editor, target), true, `${label}/${target}: 点击未处理`)
              assert.deepEqual(textOrder(editor.state.doc), texts, `${label}/${target}: 文本顺序变化`)
              assert.equal(selectedText(editor), selectedBefore, `${label}/${target}: 选区漂移`)
              for (const text of ['P1', 'P2', 'P3']) {
                assert.deepEqual(contextForText(editor, text), expected == null
                  ? { listTypes: [], itemTypes: [] }
                  : { listTypes: [expected], itemTypes: [ITEM_TYPES[expected]] }, `${label}/${target}: ${text} 状态错误，期望 ${expected}`)
              }
              assert.deepEqual(contextForText(editor, 'P0'), {
                listTypes: [sourceType],
                itemTypes: [ITEM_TYPES[sourceType]],
              }, `${label}/${target}: 未选中 P0 被影响`)
              assert.deepEqual(contextForText(editor, 'P4'), {
                listTypes: [sourceType],
                itemTypes: [ITEM_TYPES[sourceType]],
              }, `${label}/${target}: 未选中 P4 被影响`)
            }
          } catch (error) {
            failures.push(error.message)
          }
          editor.destroy()
        }
      }
    }
  }
  assert.deepEqual(failures, [], `三连按钮序列失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

test('按钮与 Tab/Shift+Tab 交叉 × 3 种列表：取消、重应用、缩进、反缩进、再取消', () => {
  const failures = []
  for (const type of LIST_TYPES) {
    const { editor, window } = createEditor({
      type: 'doc',
      content: [list(type, ['K0', 'K1', 'K2'].map((text) => item(type, text)))],
    })
    const label = type
    try {
      setCursor(editor, 'K1', 2)
      assert.equal(toggleActiveListFormatting(editor, type), true, `${label}: 初始取消失败`)
      assert.deepEqual(contextForText(editor, 'K1'), { listTypes: [], itemTypes: [] }, `${label}: 初始取消仍在列表`)
      assert.deepEqual(contextForText(editor, 'K0'), { listTypes: [type], itemTypes: [ITEM_TYPES[type]] }, `${label}: 初始取消影响 K0`)
      assert.deepEqual(contextForText(editor, 'K2'), { listTypes: [type], itemTypes: [ITEM_TYPES[type]] }, `${label}: 初始取消影响 K2`)

      assert.equal(toggleActiveListFormatting(editor, type), true, `${label}: 重应用失败`)
      assert.deepEqual(contextForText(editor, 'K1'), { listTypes: [type], itemTypes: [ITEM_TYPES[type]] }, `${label}: 重应用状态错误`)

      const sink = pressTab(editor, window)
      assert.equal(sink.defaultPrevented, true, `${label}: Tab 未由列表处理`)
      assert.equal(contextForText(editor, 'K1').listTypes.length, 2, `${label}: Tab 后应缩进一层`)

      const lift = pressTab(editor, window, true)
      assert.equal(lift.defaultPrevented, true, `${label}: Shift+Tab 未由列表处理`)
      assert.equal(contextForText(editor, 'K1').listTypes.length, 1, `${label}: Shift+Tab 后应回到一层`)

      assert.equal(toggleActiveListFormatting(editor, type), true, `${label}: 键盘往返后取消失败`)
      assert.deepEqual(contextForText(editor, 'K1'), { listTypes: [], itemTypes: [] }, `${label}: 键盘往返后取消仍有列表`)
      assert.deepEqual(textOrder(editor.state.doc), ['K0', 'K1', 'K2'], `${label}: 键盘往返后文本顺序变化`)
    } catch (error) {
      failures.push(error.message)
    }
    editor.destroy()
  }
  assert.deepEqual(failures, [], `按钮/键盘交叉矩阵失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

test('按钮转换不能把带深后代的视觉子列表推到第 4 层', () => {
  const failures = []
  for (const sourceType of LIST_TYPES) {
    const targetType = LIST_TYPES[(LIST_TYPES.indexOf(sourceType) + 1) % LIST_TYPES.length]
    const deep = list('orderedList', [
      item('orderedList', 'B1', [
        list('taskList', [item('taskList', 'B1a')]),
      ]),
    ])
    const { editor } = createEditor({
      type: 'doc',
      content: [list(sourceType, [
        item(sourceType, 'A'),
        item(sourceType, 'B', [deep], { dvnIndentLevel: 1 }),
        item(sourceType, 'C', [], { dvnIndentLevel: 1 }),
      ])],
    })
    const maxVisualDepth = () => {
      let maxDepth = 0
      const walk = (node, structuralDepth = 0) => {
        if (LIST_TYPES.includes(node.type?.name)) {
          node.forEach((child) => {
            if (child.type?.name !== 'listItem' && child.type?.name !== 'taskItem') return
            const visualDepth = structuralDepth + 1 + (Number(child.attrs?.dvnIndentLevel) || 0)
            maxDepth = Math.max(maxDepth, visualDepth)
            walk(child, structuralDepth + 1)
          })
          return
        }
        node.forEach?.((child) => walk(child, structuralDepth))
      }
      walk(editor.state.doc)
      return maxDepth
    }
    const label = `${sourceType}->${targetType}`
    try {
      setCursor(editor, 'A', 1)
      assert.equal(toggleActiveListFormatting(editor, targetType), true, `${label}: 转换未处理`)
      assert.deepEqual(textOrder(editor.state.doc), ['A', 'B', 'B1', 'B1a', 'C'], `${label}: 文本顺序变化`)
      assert.ok(maxVisualDepth() <= 3, `${label}: 转换后出现第 ${maxVisualDepth()} 层列表`)
      assert.deepEqual(contextForText(editor, 'A'), {
        listTypes: [targetType],
        itemTypes: [ITEM_TYPES[targetType]],
      }, `${label}: 目标行类型错误`)
      assert.deepEqual(contextForText(editor, 'B').listTypes, [sourceType], `${label}: 未选中的视觉子项被挂进了目标行`)
      assert.equal(editor.state.selection.empty, true, `${label}: 转换后光标不应变成范围选区`)
      assert.equal(editor.state.selection.$from.parent.textContent, 'A', `${label}: 光标离开目标行`)
    } catch (error) {
      failures.push(error.message)
    }
    editor.destroy()
  }
  assert.deepEqual(failures, [], `深后代视觉子列表转换失败 ${failures.length} 例：\n${failures.join('\n')}`)
})


test('三级列表项内的后续段落应用按钮时保持三级，不制造第 4 层', () => {
  const failures = []
  for (const targetType of LIST_TYPES) {
    const { editor } = createEditor({
      type: 'doc',
      content: [list('taskList', [
        item('taskList', 'A', [
          list('orderedList', [
            item('orderedList', 'B', [
              list('bulletList', [
                item('bulletList', 'C', [paragraph('D')]),
              ]),
            ]),
          ]),
        ]),
      ])],
    })
    const label = `三级后续段落 -> ${targetType}`
    try {
      setCursor(editor, 'D', 1)
      const beforeOrder = textOrder(editor.state.doc)
      assert.equal(toggleActiveListFormatting(editor, targetType), true, `${label}: 按钮未处理`)
      assert.deepEqual(textOrder(editor.state.doc), beforeOrder, `${label}: 文本顺序变化`)

      const contexts = contextForText(editor, 'D')
      assert.equal(contexts.listTypes.at(-1), targetType, `${label}: 目标段落未应用目标列表`)
      assert.equal(contexts.listTypes.length, 3, `${label}: 目标段落不在三级`)
      assert.equal(contextForText(editor, 'C').listTypes.length, 3, `${label}: 未选中的前一段落层级漂移`)
      assert.equal(editor.state.selection.empty, true, `${label}: 光标被扩成选区`)
      assert.equal(editor.state.selection.$from.parent.textContent, 'D', `${label}: 光标离开目标段落`)
      assert.equal(editor.state.selection.$from.parentOffset, 1, `${label}: 光标偏移漂移`)

      let maxVisualDepth = 0
      const walk = (node, structuralDepth = 0) => {
        if (LIST_TYPES.includes(node.type?.name)) {
          node.forEach((child) => {
            const indent = Number(child.attrs?.dvnIndentLevel) || 0
            maxVisualDepth = Math.max(maxVisualDepth, structuralDepth + 1 + indent)
            walk(child, structuralDepth + 1)
          })
          return
        }
        node.forEach?.((child) => walk(child, structuralDepth))
      }
      walk(editor.state.doc)
      assert.ok(maxVisualDepth <= 3, `${label}: 出现第 ${maxVisualDepth} 层列表`)
    } catch (error) {
      failures.push(error.message)
    }
    editor.destroy()
  }
  assert.deepEqual(failures, [], `三级后续段落按钮矩阵失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

test('Tab 把独立列表挂到前一项时，深子树不能产生第 4 层', () => {
  const failures = []
  const sourceTypes = LIST_TYPES
  const childTypes = LIST_TYPES
  for (const previousType of sourceTypes) {
    for (const targetType of LIST_TYPES) {
      for (const childType of childTypes) {
        const deepChild = childType === 'taskList'
          ? item('taskList', 'T3', [], { checked: true })
          : item(childType, 'T3')
        const middle = childType === 'taskList'
          ? item('taskList', 'T2', [list('orderedList', [item('orderedList', 'T3')])], { checked: true })
          : item(childType, 'T2', [list('orderedList', [item('orderedList', 'T3')])])
        const target = targetType === 'taskList'
          ? item('taskList', 'T', [
            list('taskList', [item('taskList', 'T1', [], { checked: true })]),
            list(childType, [middle]),
          ])
          : item(targetType, 'T', [
            list('taskList', [item('taskList', 'T1', [], { checked: true })]),
            list(childType, [middle]),
          ])
        const previous = previousType === 'taskList'
          ? item('taskList', 'P', [], { checked: false })
          : item(previousType, 'P')
        const { editor, window } = createEditor({
          type: 'doc',
          content: [
            list(previousType, [previous]),
            list(targetType, [target]),
          ],
        })
        const label = `${previousType}/${targetType}/${childType}`
        const beforeOrder = textOrder(editor.state.doc)
        setCursor(editor, 'T', 1)
        const event = pressTab(editor, window)
        let maxVisualDepth = 0
        const walk = (node, structuralDepth = 0) => {
          if (LIST_TYPES.includes(node.type?.name)) {
            node.forEach((child) => {
              const indent = Number(child.attrs?.dvnIndentLevel) || 0
              maxVisualDepth = Math.max(maxVisualDepth, structuralDepth + 1 + indent)
              walk(child, structuralDepth + 1)
            })
            return
          }
          node.forEach?.((child) => walk(child, structuralDepth))
        }
        walk(editor.state.doc)
        try {
          assert.equal(event.defaultPrevented, true, `${label}: Tab 未处理`)
          assert.deepEqual(textOrder(editor.state.doc), beforeOrder, `${label}: 文本顺序变化`)
          assert.ok(maxVisualDepth <= 3, `${label}: 出现第 ${maxVisualDepth} 层列表`)
          assert.equal(getVisualDepthForText(editor, 'T'), 2, `${label}: 目标项没有缩进到二级`)
          assert.equal(editor.state.selection.empty, true, `${label}: 光标被扩成选区`)
          assert.equal(editor.state.selection.$from.parent.textContent, 'T', `${label}: 光标离开目标项`)
          assert.equal(editor.state.selection.$from.parentOffset, 1, `${label}: 光标偏移漂移`)
        } catch (error) {
          failures.push(error.message)
        }
        editor.destroy()
      }
    }
  }
  assert.deepEqual(failures, [], `独立列表 Tab 深度矩阵失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

test('独立列表挂回前一项时，多段落项内任意段落的光标都必须保持', () => {
  const failures = []
  for (const previousType of LIST_TYPES) {
    for (const targetType of LIST_TYPES) {
      for (const paragraphIndex of [0, 1, 2]) {
        const paragraphs = ['T0', 'T1', 'T2'].map(paragraph)
        const { editor, window } = createEditor({
          type: 'doc',
          content: [
            list(previousType, [item(previousType, 'P', [], { dvnIndentLevel: 1 })]),
            list(targetType, [item(targetType, '', paragraphs)]),
          ],
        })
        const targetText = `T${paragraphIndex}`
        const label = `${previousType}/${targetType}/T${paragraphIndex}`
        try {
          setCursor(editor, targetText, 1)
          const beforeOrder = textOrder(editor.state.doc)
          const event = pressTab(editor, window)
          assert.equal(event.defaultPrevented, true, `${label}: Tab 未处理`)
          assert.deepEqual(textOrder(editor.state.doc), beforeOrder, `${label}: 文本顺序变化`)
          assert.equal(editor.state.selection.empty, true, `${label}: 光标被扩成选区`)
          assert.equal(editor.state.selection.$from.parent.textContent, targetText, `${label}: 光标跳到其它段落`)
          assert.equal(editor.state.selection.$from.parentOffset, 1, `${label}: 光标偏移漂移`)
          assert.equal(getVisualDepthForText(editor, 'P'), 2, `${label}: 前项视觉层级错误`)
        } catch (error) {
          failures.push(error.message)
        }
        editor.destroy()
      }
    }
  }
  assert.deepEqual(failures, [], `独立列表多段落光标矩阵失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

test('视觉缩进 Shift+Tab/Tab 往返 × 3 种列表 × 多段落位置：光标不能被旧快照拉回', () => {
  const failures = []
  const text = 'CaretText'
  const positions = [
    [1, 0],
    [4, 2],
    [text.length, 5],
    [8, text.length],
  ]
  for (const type of LIST_TYPES) {
    for (const paragraphIndex of [0, 1]) {
      for (const [initialOffset, restoredOffset] of positions) {
        const targetItem = paragraphIndex === 0
          ? item(type, text, [], { dvnIndentLevel: 1 })
          : item(type, 'Head', [paragraph(text)], { dvnIndentLevel: 1 })
        const { editor, window } = createEditor({
          type: 'doc',
          content: [list(type, [
            targetItem,
            item(type, 'Sibling', [], { dvnIndentLevel: 2 }),
          ])],
        })
        const label = `${type}/p${paragraphIndex}/${initialOffset}->${restoredOffset}`
        const caret = () => ({
          text: editor.state.selection.$from.parent.textContent,
          offset: editor.state.selection.$from.parentOffset,
          empty: editor.state.selection.empty,
        })
        try {
          setCursor(editor, text, initialOffset)
          assert.equal(pressTab(editor, window, true).defaultPrevented, true, `${label}: Shift+Tab 未处理`)
          assert.deepEqual(caret(), { text, offset: initialOffset, empty: true }, `${label}: Shift+Tab 后光标漂移`)
          assert.equal(getVisualDepthForText(editor, text), 1, `${label}: 反缩进后未回到一级`)

          // 模拟用户在反缩进后重新定位光标，再按 Tab 恢复列表层级。
          setCursor(editor, text, restoredOffset)
          assert.equal(pressTab(editor, window).defaultPrevented, true, `${label}: Tab 未处理`)
          assert.deepEqual(caret(), { text, offset: restoredOffset, empty: true }, `${label}: Tab 恢复了旧光标位置`)
          assert.equal(getVisualDepthForText(editor, text), 2, `${label}: Tab 后未恢复到视觉二级`)
        } catch (error) {
          failures.push(error.message)
        }
        editor.destroy()
      }
    }
  }
  assert.deepEqual(failures, [], `视觉缩进光标矩阵失败 ${failures.length} 例：\n${failures.join('\n')}`)
})

function getVisualDepthForText(editor, text) {
  const range = findText(editor, text)
  const $pos = editor.state.doc.resolve(range.from)
  let structuralDepth = 0
  let indent = 0
  for (let depth = 1; depth <= $pos.depth; depth += 1) {
    const node = $pos.node(depth)
    if (node.type.name === 'listItem' || node.type.name === 'taskItem') {
      structuralDepth += 1
      indent = Number(node.attrs?.dvnIndentLevel) || 0
    }
  }
  return structuralDepth + indent
}

test('三级项内中间段落转换时保持文本顺序，不能被释放到后续段落之后', () => {
  for (const targetType of LIST_TYPES) {
    const { editor } = createEditor({
      type: 'doc',
      content: [list('taskList', [
        item('taskList', 'A', [
          list('bulletList', [
            item('bulletList', 'B', [
              list('bulletList', [
                item('bulletList', 'C0', [paragraph('C1'), paragraph('C2')]),
              ]),
            ]),
          ]),
        ]),
      ])],
    })
    const beforeOrder = textOrder(editor.state.doc)
    setCursor(editor, 'C1', 1)
    const beforeSelection = {
      text: editor.state.selection.$from.parent.textContent,
      offset: editor.state.selection.$from.parentOffset,
    }

    assert.equal(toggleActiveListFormatting(editor, targetType), true, `${targetType}: 按钮未处理`)
    assert.deepEqual(textOrder(editor.state.doc), beforeOrder, `${targetType}: 文本顺序变成 C0/C2/C1`)
    assert.deepEqual(contextForText(editor, 'C0'), {
      listTypes: ['taskList', 'bulletList', 'bulletList'],
      itemTypes: ['taskItem', 'listItem', 'listItem'],
    }, `${targetType}: 未选中的 C0 层级/类型漂移`)
    assert.deepEqual(contextForText(editor, 'C1'), {
      listTypes: ['taskList', 'bulletList', targetType],
      itemTypes: ['taskItem', 'listItem', ITEM_TYPES[targetType]],
    }, `${targetType}: 目标段落未成为三级目标列表项`)
    assert.equal(contextForText(editor, 'C2').listTypes.length <= 3, true, `${targetType}: C2 出现第 4 层`)
    assert.equal(editor.state.selection.empty, true, `${targetType}: 光标被扩成选区`)
    assert.deepEqual({
      text: editor.state.selection.$from.parent.textContent,
      offset: editor.state.selection.$from.parentOffset,
    }, beforeSelection, `${targetType}: 光标离开目标段落或偏移漂移`)
    editor.destroy()
  }
})
