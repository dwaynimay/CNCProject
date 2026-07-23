/**
 * telegramService.js — Pengirim notifikasi ke Telegram Bot API.
 * Memakai fetch bawaan Node (>=18). Tidak butuh dependency tambahan.
 */
require('dotenv').config();

const ENABLED  = String(process.env.TELEGRAM_ENABLED).toLowerCase() === 'true';
const TOKEN    = process.env.TELEGRAM_BOT_TOKEN || '';
const CHAT_ID  = process.env.TELEGRAM_CHAT_ID   || '';

const API_URL = TOKEN ? `https://api.telegram.org/bot${TOKEN}/sendMessage` : '';

function isConfigured() {
    return ENABLED && TOKEN && CHAT_ID;
}

/**
 * Kirim satu pesan ke Telegram.
 * @param {string} text  Teks pesan (mendukung Markdown).
 * @returns {Promise<{ok:boolean, skipped?:boolean, error?:string}>}
 */
async function sendMessage(text) {
    if (!isConfigured()) {
        // Diam-diam skip supaya sistem tetap jalan tanpa konfigurasi Telegram.
        return { ok: false, skipped: true };
    }

    try {
        const res = await fetch(API_URL, {
            method:  'POST',
            headers: { 'Content-Type': 'application/json' },
            body:    JSON.stringify({
                chat_id:                  CHAT_ID,
                text,
                parse_mode:               'Markdown',
                disable_web_page_preview: true,
            }),
        });

        const json = await res.json().catch(() => ({}));
        if (!res.ok || json.ok === false) {
            const msg = json.description || `HTTP ${res.status}`;
            console.error('[Telegram] Gagal kirim:', msg);
            return { ok: false, error: msg };
        }
        return { ok: true };
    } catch (err) {
        console.error('[Telegram] Error jaringan:', err.message);
        return { ok: false, error: err.message };
    }
}

module.exports = { sendMessage, isConfigured };
