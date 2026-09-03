// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

import Image from '@tiptap/extension-image'

const IMAGE_ATTRS = ['src', 'alt', 'title', 'width', 'height', 'data-rel-path']

function imageDomAttributes(baseAttributes, nodeAttrs) {
  return {
    ...baseAttributes,
    src: nodeAttrs?.src,
    alt: nodeAttrs?.alt,
    title: nodeAttrs?.title,
    width: nodeAttrs?.width,
    height: nodeAttrs?.height,
    'data-rel-path': nodeAttrs?.relPath,
  }
}

function applyImageAttributes(img, attributes) {
  for (const name of IMAGE_ATTRS) {
    img.removeAttribute(name)
  }

  for (const [name, value] of Object.entries(attributes)) {
    if (value == null || value === false) continue
    if (name === 'relPath') continue
    img.setAttribute(name, String(value))
  }
}

export const ImageBlock = Image.extend({
  name: 'image',

  addOptions() {
    return {
      ...this.parent?.(),
      inline: true,
    }
  },
  addAttributes() {
    return {
      ...this.parent?.(),
      relPath: {
        default: null,
        parseHTML: element => element.getAttribute('data-rel-path'),
        renderHTML: attributes => attributes.relPath
          ? { 'data-rel-path': attributes.relPath }
          : {},
      },
    }
  },

  addNodeView() {
    const baseAttributes = this.options.HTMLAttributes || {}
    return ({ node }) => {
      const wrapper = document.createElement('span')
      wrapper.className = 'dvn-image-node'
      wrapper.setAttribute('data-dvn-image-node', 'true')

      const img = document.createElement('img')
      applyImageAttributes(img, imageDomAttributes(baseAttributes, node.attrs))

      const selection = document.createElement('span')
      selection.className = 'dvn-image-node-selection'
      selection.setAttribute('data-testid', 'tiptap-image-selection')
      selection.setAttribute('aria-hidden', 'true')

      wrapper.appendChild(img)
      wrapper.appendChild(selection)

      return {
        dom: wrapper,
        update(updatedNode) {
          if (updatedNode.type.name !== node.type.name) return false
          node = updatedNode
          applyImageAttributes(img, imageDomAttributes(baseAttributes, node.attrs))
          return true
        },
        selectNode() {
          wrapper.classList.add('ProseMirror-selectednode', 'dvn-image-node-selected')
        },
        deselectNode() {
          wrapper.classList.remove('ProseMirror-selectednode', 'dvn-image-node-selected')
        },
        ignoreMutation() {
          return true
        },
      }
    }
  },
})
