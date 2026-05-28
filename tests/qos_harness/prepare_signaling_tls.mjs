import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';

const repoRoot = path.resolve(path.dirname(new URL(import.meta.url).pathname), '..', '..');
const certDir = '/opt/mediasoup-cpp/certs';
const certPath = path.join(certDir, 'tls.pem');
const keyPath = path.join(certDir, 'tls.key');
const repoCertPath = path.join(repoRoot, 'docker', '_.zelostech.com.cn.pem');
const repoKeyPath = path.join(repoRoot, 'docker', '_.zelostech.com.cn.key');

export function ensureSignalingTlsFiles() {
  fs.mkdirSync(certDir, { recursive: true });

  if (fs.existsSync(repoCertPath) && fs.existsSync(repoKeyPath)) {
    fs.copyFileSync(repoCertPath, certPath);
    fs.copyFileSync(repoKeyPath, keyPath);
    fs.chmodSync(certPath, 0o644);
    fs.chmodSync(keyPath, 0o600);
    return;
  }

  if (fs.existsSync(certPath) && fs.existsSync(keyPath)) {
    return;
  }

  execFileSync('openssl', [
    'req',
    '-x509',
    '-newkey',
    'rsa:2048',
    '-sha256',
    '-nodes',
    '-days',
    '7',
    '-subj',
    '/CN=127.0.0.1',
    '-addext',
    'subjectAltName=IP:127.0.0.1,DNS:localhost',
    '-keyout',
    keyPath,
    '-out',
    certPath,
  ], { stdio: 'ignore' });
}
