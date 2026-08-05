// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Entry module for `node --import`: registers the CSS loader hook below so
// that validate-envelope.mjs can import create-schema.js (which transitively
// imports voice-block.css?inline) without a bundler.
import { register } from 'node:module';
import { pathToFileURL } from 'node:url';

const hookURL = new URL('./css-loader-hook.mjs', import.meta.url);
register(pathToFileURL(hookURL.pathname));
