import http from 'node:http';
import https from 'node:https';
import net from 'node:net';
import tls from 'node:tls';

const insecureTlsOptions = {
  rejectUnauthorized: false,
};

export function createWebSocketConnection(urlObject) {
  const protocol = urlObject.protocol;
  const host = urlObject.hostname;
  const port = Number.parseInt(urlObject.port || (protocol === 'wss:' ? '443' : '80'), 10);

  if (protocol === 'wss:') {
    return tls.connect({
      host,
      port,
      ...insecureTlsOptions,
    });
  }

  return net.createConnection({ host, port });
}

export function websocketOriginForUrl(urlObject) {
  return urlObject.protocol === 'wss:'
    ? `https://${urlObject.hostname}`
    : `http://${urlObject.hostname}`;
}

export function httpGetJson(urlObject, path) {
  const client = urlObject.protocol === 'https:' ? https : http;
  const requestUrl = new URL(path, urlObject);
  return new Promise((resolve, reject) => {
    const req = client.get(requestUrl, insecureTlsOptions, res => {
      const chunks = [];
      res.on('data', chunk => chunks.push(chunk));
      res.on('end', () => {
        const body = Buffer.concat(chunks).toString('utf8');
        try {
          resolve({ status: res.statusCode ?? 0, body, json: body ? JSON.parse(body) : {} });
        } catch (error) {
          reject(new Error(`failed to parse ${path}: ${error.message}; body=${body.slice(0, 200)}`));
        }
      });
    });
    req.on('error', reject);
  });
}
