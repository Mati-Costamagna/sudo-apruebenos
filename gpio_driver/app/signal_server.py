#!/usr/bin/env python3
"""
signal_server.py — Servidor web para visualizar señales GPIO desde la BBB.

Corre en la BeagleBone Black. Lee /dev/SdC_gpio periódicamente y sirve
los datos por HTTP. La interfaz gráfica se abre desde el navegador de la PC.

Uso en la BBB:
    python3 signal_server.py [--device /dev/SdC_gpio] [--port 8080] [--interval 100]

Desde la PC, abrir en el navegador:
    http://<IP_BBB>:8080
"""

import argparse
import collections
import http.server
import json
import threading
import time

# ─── Configuración ────────────────────────────────────────────────────────────

MAX_POINTS = 300  # cantidad de muestras visibles en el gráfico

# ─── Buffer compartido ────────────────────────────────────────────────────────

data_lock  = threading.Lock()
timestamps = collections.deque(maxlen=MAX_POINTS)
ch0_values = collections.deque(maxlen=MAX_POINTS)
ch1_values = collections.deque(maxlen=MAX_POINTS)

# ─── Hilo de muestreo ─────────────────────────────────────────────────────────

def sampler(device: str, interval_ms: int) -> None:
    """Lee ambos canales del driver periódicamente y acumula las muestras."""
    period = interval_ms / 1000.0
    while True:
        t = time.time()
        v0 = read_channel(device, 0)
        v1 = read_channel(device, 1)
        with data_lock:
            timestamps.append(t)
            ch0_values.append(v0)
            ch1_values.append(v1)
        time.sleep(period)


def read_channel(device: str, channel: int) -> int:
    """Selecciona el canal y lee el estado del pin. Retorna 0, 1 o -1 si falla."""
    try:
        with open(device, 'w') as f:
            f.write(str(channel))
        with open(device, 'r') as f:
            return int(f.read().strip())
    except Exception:
        return -1


# ─── Servidor HTTP ────────────────────────────────────────────────────────────

class Handler(http.server.BaseHTTPRequestHandler):

    def do_GET(self):
        if self.path == '/':
            self._serve_html()
        elif self.path == '/data':
            self._serve_data()
        else:
            self.send_error(404)

    def _serve_html(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(HTML.encode())

    def _serve_data(self):
        with data_lock:
            if not timestamps:
                payload = {'times': [], 'ch0': [], 'ch1': []}
            else:
                t0 = timestamps[0]
                payload = {
                    'times': [round(t - t0, 3) for t in timestamps],
                    'ch0':   list(ch0_values),
                    'ch1':   list(ch1_values),
                }
        body = json.dumps(payload).encode()
        self.send_response(200)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        pass  # silenciar el log de acceso en consola


# ─── Página HTML embebida ─────────────────────────────────────────────────────

HTML = """<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <title>GPIO Monitor — BeagleBone Black</title>
  <script src="https://cdn.jsdelivr.net/npm/chart.js@4"></script>
  <style>
    body { font-family: monospace; background: #1e1e1e; color: #d4d4d4; margin: 0; padding: 20px; }
    h1   { font-size: 1.2em; color: #9cdcfe; margin-bottom: 4px; }
    p    { color: #808080; font-size: 0.85em; margin-top: 0; }
    .container { max-width: 960px; margin: auto; }
    canvas { background: #252526; border-radius: 4px; }
    .controls { margin: 12px 0; }
    button {
      background: #3c3c3c; color: #d4d4d4; border: 1px solid #555;
      padding: 6px 16px; margin-right: 8px; cursor: pointer; border-radius: 3px;
      font-family: monospace;
    }
    button.active { background: #0e639c; border-color: #0e639c; color: #fff; }
    .status { font-size: 0.8em; color: #608b4e; margin-top: 8px; }
  </style>
</head>
<body>
<div class="container">
  <h1>GPIO Monitor — BeagleBone Black</h1>
  <p>CDD: /dev/SdC_gpio &nbsp;|&nbsp; GPIO1[28] (canal 0) &nbsp;/&nbsp; GPIO1[18] (canal 1)</p>

  <div class="controls">
    <button id="btn0" class="active" onclick="showChannel(0)">Canal 0 — GPIO1[28]</button>
    <button id="btn1"               onclick="showChannel(1)">Canal 1 — GPIO1[18]</button>
    <button onclick="clearChart()">Limpiar</button>
  </div>

  <canvas id="chart" height="100"></canvas>
  <div class="status" id="status">Conectando...</div>
</div>

<script>
const POLL_MS  = 200;
let   activeChannel = 0;

const ctx = document.getElementById('chart').getContext('2d');
const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [{
      label: 'Canal 0 — GPIO1[28]',
      data: [],
      borderColor: '#4ec9b0',
      borderWidth: 1.5,
      pointRadius: 0,
      stepped: true,       // señal digital → escalones
      fill: false,
    }]
  },
  options: {
    animation: false,
    scales: {
      x: {
        title: { display: true, text: 'Tiempo (s)', color: '#808080' },
        ticks: { color: '#808080', maxTicksLimit: 10 },
        grid:  { color: '#333' }
      },
      y: {
        title: { display: true, text: 'Nivel lógico', color: '#808080' },
        ticks: {
          color: '#808080',
          callback: v => v === 1 ? 'HIGH (1)' : v === 0 ? 'LOW (0)' : ''
        },
        min: -0.1, max: 1.1,
        grid: { color: '#333' }
      }
    },
    plugins: { legend: { labels: { color: '#d4d4d4' } } }
  }
});

function showChannel(ch) {
  activeChannel = ch;
  document.getElementById('btn0').classList.toggle('active', ch === 0);
  document.getElementById('btn1').classList.toggle('active', ch === 1);
  const colors = ['#4ec9b0', '#ce9178'];
  const labels = ['Canal 0 — GPIO1[28]', 'Canal 1 — GPIO1[18]'];
  chart.data.datasets[0].borderColor = colors[ch];
  chart.data.datasets[0].label = labels[ch];
  clearChart();
}

function clearChart() {
  chart.data.labels = [];
  chart.data.datasets[0].data = [];
  chart.update();
}

async function poll() {
  try {
    const res  = await fetch('/data');
    const json = await res.json();
    const vals = activeChannel === 0 ? json.ch0 : json.ch1;

    chart.data.labels = json.times.map(t => t.toFixed(2));
    chart.data.datasets[0].data = vals;
    chart.update('none');

    const last = vals[vals.length - 1];
    document.getElementById('status').textContent =
      `Muestras: ${vals.length} | Último valor: ${last === 1 ? 'HIGH (1)' : last === 0 ? 'LOW (0)' : 'error'} | t=${json.times[json.times.length-1]?.toFixed(2)}s`;
  } catch(e) {
    document.getElementById('status').textContent = 'Error de conexión — reintentando...';
  }
  setTimeout(poll, POLL_MS);
}

poll();
</script>
</body>
</html>"""


# ─── Main ─────────────────────────────────────────────────────────────────────

def main():
    parser = argparse.ArgumentParser(description='Servidor web GPIO para BBB')
    parser.add_argument('--device',   default='/dev/SdC_gpio', help='Ruta al CDD')
    parser.add_argument('--port',     type=int, default=8080,  help='Puerto HTTP (default: 8080)')
    parser.add_argument('--interval', type=int, default=100,   help='Período de muestreo en ms (default: 100)')
    args = parser.parse_args()

    print(f'Device : {args.device}')
    print(f'Puerto : http://0.0.0.0:{args.port}')
    print(f'Muestreo: {args.interval} ms ({1000 // args.interval} Hz)')
    print(f'Abrir en la PC: http://<IP_BBB>:{args.port}')

    t = threading.Thread(target=sampler, args=(args.device, args.interval), daemon=True)
    t.start()

    server = http.server.HTTPServer(('0.0.0.0', args.port), Handler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print('\nServidor detenido.')


if __name__ == '__main__':
    main()
