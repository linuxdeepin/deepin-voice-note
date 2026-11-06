// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import assert from 'node:assert/strict'
import { readFileSync } from 'node:fs'
import test from 'node:test'

const itemListView = readFileSync(new URL('../../src/gui/mainwindow/ItemListView.qml', import.meta.url), 'utf8')
const mainWindow = readFileSync(new URL('../../src/gui/mainwindow/MainWindow.qml', import.meta.url), 'utf8')

test('search titles use rich text while editing and accessibility use plain titles', () => {
  assert.match(itemListView, /property string plainTitle: model\.name \|\| ""/)
  assert.match(itemListView, /Accessible\.name: rootItemDelegate\.plainTitle/)
  assert.match(itemListView, /highlightedTitle: notesData\[i\]\.highlightedTitle \|\| ""/)
  assert.match(itemListView, /property string displayTitle: \{[\s\S]*?!isSearch[\s\S]*?model\.highlightedTitle[\s\S]*?makeHighlightedTitle/)
  assert.match(mainWindow, /name: notesData\[i\]\.name,[\s\S]*?highlightedTitle: notesData\[i\]\.highlightedTitle \|\| ""/)
  assert.match(itemListView, /id: noteNameLabel[\s\S]*?text: rootItemDelegate\.displayTitle[\s\S]*?textFormat: isSearch \? Text\.RichText : Text\.PlainText/)
  assert.match(itemListView, /id: itemNameTip[\s\S]*?text: rootItemDelegate\.plainTitle/)
  assert.match(itemListView, /id: itemNameTip[\s\S]*?contentItem: Text \{[\s\S]*?textFormat: Text\.PlainText/)
  assert.match(itemListView, /id: renameLine[\s\S]*?text: rootItemDelegate\.plainTitle/)
  assert.match(itemListView, /VNoteMainManager\.makeHighlightedTitle\(newTitle\)/)
})
