#!/usr/bin/env bash

set -u

APP_NAME="deepin-voice-note"

if [[ -z "${HOME:-}" || "${HOME}" == "/" ]]; then
    echo "Invalid HOME: ${HOME:-<empty>}" >&2
    exit 1
fi

DATA_DIR="${HOME}/.local/share/deepin/deepin-voice-note"
CONFIG_DIR="${HOME}/.config/deepin/deepin-voice-note"
DB_PATH="${DATA_DIR}/deepin-voice-note1.0.db"
LOG_FILE="$(mktemp "${TMPDIR:-/tmp}/deepin-voice-note-init.XXXXXX.log")"

cleanup_log()
{
    rm -f "${LOG_FILE}"
}
trap cleanup_log EXIT

stop_app()
{
    killall -q "${APP_NAME}" 2>/dev/null || true
}

wait_for_schema()
{
    local i
    for i in {1..80}; do
        if [[ -f "${DB_PATH}" ]] \
            && [[ "$(sqlite3 "${DB_PATH}" \
                "SELECT name FROM sqlite_master WHERE type='table' AND name='vnote_folder_tbl';" \
                2>/dev/null || true)" == "vnote_folder_tbl" ]]; then
            return 0
        fi
        sleep 0.1
    done

    echo "Timed out waiting for ${APP_NAME} database schema" >&2
    return 1
}

seed_default_notebook()
{
    sqlite3 "${DB_PATH}" <<'SQL'
INSERT INTO vnote_folder_tbl(
    folder_id,
    category_id,
    folder_name,
    default_icon,
    folder_state,
    max_noteid
) VALUES(1, 0, '记事本1', 0, 0, 1);

INSERT INTO vnote_items_tbl(
    note_id,
    folder_id,
    note_type,
    note_title,
    meta_data,
    note_state
) VALUES(
    1,
    1,
    0,
    '文本',
    '{"htmlCode":"<p><br></p>"}',
    0
);
SQL
}

stop_app
rm -rf -- "${DATA_DIR}" "${CONFIG_DIR}"

"${APP_NAME}" >"${LOG_FILE}" 2>&1 &
INIT_PID=$!

if ! wait_for_schema; then
    kill "${INIT_PID}" 2>/dev/null || true
    stop_app
    exit 1
fi

kill "${INIT_PID}" 2>/dev/null || true
stop_app

seed_default_notebook

exec "${APP_NAME}"
