#!/usr/bin/env bash

set -euo pipefail

APP_NAME="deepin-voice-note"

if [[ -z "${HOME:-}" || "${HOME}" == "/" ]]; then
    echo "Invalid HOME: ${HOME:-<empty>}" >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

DATA_DIR="${HOME}/.local/share/deepin/deepin-voice-note"
CONFIG_DIR="${HOME}/.config/deepin/deepin-voice-note"
DB_PATH="${DATA_DIR}/deepin-voice-note1.0.db"
FIXTURE_DB="${PROJECT_ROOT}/tests/at/fixtures/deepin-voice-note1.0.db"

stop_app()
{
    killall -q "${APP_NAME}" 2>/dev/null || true
}

clean_qml_cache()
{
    # AT 每次安装/切换新包后，旧 QML 磁盘缓存可能继续引用旧组件名，
    # 导致 QML 引擎加载失败并被 youqu 误报为“应用程序未启动”。
    # 清理当前应用缓存并禁用本次进程的 QML disk cache，保证读取最新 qrc 资源。
    local cache_dir="${HOME}/.cache/deepin/${APP_NAME}/qmlcache"

    if [[ -d "${cache_dir}" ]]; then
        echo "Clean QML cache: ${cache_dir}"
        rm -rf -- "${cache_dir}"
    fi

    export QML_DISABLE_DISK_CACHE=1
}

wait_for_schema()
{
    local table_count

    table_count="$(sqlite3 "${DB_PATH}" \
        "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name IN ('vnote_folder_tbl', 'vnote_items_tbl', 'vnote_category_tbl');")"

    if [[ "${table_count}" != "3" ]]; then
        echo "Invalid AT fixture database schema: expected 3 tables, got ${table_count}" >&2
        return 1
    fi
}

verify_fixture_data()
{
    local folder_count
    local note_count

    folder_count="$(sqlite3 "${DB_PATH}" \
        "SELECT COUNT(*) FROM vnote_folder_tbl WHERE folder_id=1 AND folder_name='记事本1' AND folder_state=0;")"
    note_count="$(sqlite3 "${DB_PATH}" \
        "SELECT COUNT(*) FROM vnote_items_tbl WHERE note_id=1 AND folder_id=1 AND note_title='文本' AND note_state=0;")"

    if [[ "${folder_count}" != "1" || "${note_count}" != "1" ]]; then
        echo "Invalid AT fixture database data: folder_count=${folder_count}, note_count=${note_count}" >&2
        return 1
    fi
}

if ! command -v sqlite3 >/dev/null 2>&1; then
    echo "sqlite3 is required for AT fixture validation" >&2
    exit 1
fi

if [[ ! -f "${FIXTURE_DB}" ]]; then
    echo "AT fixture database not found: ${FIXTURE_DB}" >&2
    exit 1
fi

stop_app
clean_qml_cache
rm -rf -- "${DATA_DIR}" "${CONFIG_DIR}"
mkdir -p -- "${DATA_DIR}" "${CONFIG_DIR}"

cp -f -- "${FIXTURE_DB}" "${DB_PATH}"
wait_for_schema
verify_fixture_data

exec "${APP_NAME}"
