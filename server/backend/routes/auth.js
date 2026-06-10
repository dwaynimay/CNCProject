/**
 * routes/auth.js — Endpoint autentikasi
 * POST /api/auth/login  → { username, password } → { token, user }
 * GET  /api/auth/me     → info user saat ini (perlu Bearer token)
 * POST /api/auth/logout → (client cukup hapus token)
 */
const router  = require('express').Router();
const bcrypt  = require('bcryptjs');
const jwt     = require('jsonwebtoken');
const { getUserByUsername, updateLastLogin } = require('../db');

const JWT_SECRET  = process.env.JWT_SECRET  || 'cnc-iot-secret-change-me';
const JWT_EXPIRES = process.env.JWT_EXPIRES_IN || '8h';

// ── Middleware: verifikasi JWT ──────────────────────────────────────────────
function requireAuth(req, res, next) {
    const authHeader = req.headers.authorization || '';
    const token = authHeader.startsWith('Bearer ') ? authHeader.slice(7) : null;
    if (!token) return res.status(401).json({ error: 'Token diperlukan' });

    try {
        req.user = jwt.verify(token, JWT_SECRET);
        next();
    } catch (e) {
        return res.status(401).json({ error: 'Token tidak valid atau kedaluwarsa' });
    }
}

// ── POST /api/auth/login ───────────────────────────────────────────────────
router.post('/login', (req, res) => {
    const { username, password } = req.body;

    if (!username || !password) {
        return res.status(400).json({ error: 'Username dan password wajib diisi' });
    }

    const user = getUserByUsername(username.trim());
    if (!user) {
        return res.status(401).json({ error: 'Username atau password salah' });
    }

    const passwordMatch = bcrypt.compareSync(password, user.password);
    if (!passwordMatch) {
        return res.status(401).json({ error: 'Username atau password salah' });
    }

    // Update last login
    updateLastLogin(user.id);

    // Buat JWT token
    const payload = { id: user.id, username: user.username, role: user.role };
    const token = jwt.sign(payload, JWT_SECRET, { expiresIn: JWT_EXPIRES });

    console.log(`[AUTH] Login berhasil: ${user.username} (${user.role})`);

    res.json({
        ok: true,
        token,
        user: {
            id:        user.id,
            username:  user.username,
            role:      user.role,
            lastLogin: user.last_login,
        },
        expiresIn: JWT_EXPIRES,
    });
});

// ── GET /api/auth/me ───────────────────────────────────────────────────────
router.get('/me', requireAuth, (req, res) => {
    const user = getUserByUsername(req.user.username);
    if (!user) return res.status(404).json({ error: 'User tidak ditemukan' });

    res.json({
        id:        user.id,
        username:  user.username,
        role:      user.role,
        lastLogin: user.last_login,
        createdAt: user.created_at,
    });
});

// ── POST /api/auth/logout ──────────────────────────────────────────────────
// Stateless JWT: client cukup hapus token dari localStorage
router.post('/logout', requireAuth, (req, res) => {
    console.log(`[AUTH] Logout: ${req.user.username}`);
    res.json({ ok: true, message: 'Logout berhasil' });
});

module.exports = router;
module.exports.requireAuth = requireAuth;
