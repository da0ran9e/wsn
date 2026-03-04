const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');
const summaryEl = document.getElementById('summary');
const stepInfoEl = document.getElementById('stepInfo');

let model = { meta: {}, nodes: [], links: [], steps: [] };
let stepIndex = 0;

function parseKeyValueLine(line) {
  const out = {};
  const pairs = line.trim().split(/\s+/);
  for (const pair of pairs) {
    const i = pair.indexOf('=');
    if (i > 0) {
      const k = pair.slice(0, i);
      const v = pair.slice(i + 1);
      out[k] = v;
    }
  }
  return out;
}

function parseCsvNumbers(text) {
  if (!text || text === '-') return [];
  return text.split(',').map(s => Number(s.trim())).filter(n => Number.isFinite(n));
}

function parseLog(text) {
  const data = { meta: {}, nodes: [], links: [], steps: [] };
  let currentStep = null;

  for (const raw of text.split(/\r?\n/)) {
    const line = raw.trim();
    if (!line || line.startsWith('#')) continue;

    if (line.startsWith('META ')) {
      const kv = parseKeyValueLine(line.slice(5));
      Object.assign(data.meta, kv);
    } else if (line.startsWith('NODE ')) {
      const kv = parseKeyValueLine(line.slice(5));
      data.nodes.push({
        id: Number(kv.id),
        x: Number(kv.x),
        y: Number(kv.y),
        cell: Number(kv.cell),
        leader: Number(kv.leader),
        color: Number(kv.color || 0)
      });
    } else if (line.startsWith('LINK ')) {
      const kv = parseKeyValueLine(line.slice(5));
      data.links.push({
        a: Number(kv.a),
        b: Number(kv.b),
        kind: kv.kind || 'logical'
      });
    } else if (line.startsWith('STEP ')) {
      const kv = parseKeyValueLine(line.slice(5));
      currentStep = {
        t: Number(kv.t),
        suspiciousCells: parseCsvNumbers(kv.suspiciousCells),
        suspiciousNodes: parseCsvNumbers(kv.suspiciousNodes)
      };
      data.steps.push(currentStep);
    } else if (line === 'ENDSTEP') {
      currentStep = null;
    }
  }

  return data;
}

function nodeById(id) {
  return model.nodes.find(n => n.id === id);
}

function fitTransform() {
  const xs = model.nodes.map(n => n.x);
  const ys = model.nodes.map(n => n.y);
  const minX = Math.min(...xs), maxX = Math.max(...xs);
  const minY = Math.min(...ys), maxY = Math.max(...ys);
  const pad = 40;
  const w = canvas.width - 2 * pad;
  const h = canvas.height - 2 * pad;

  const sx = w / Math.max(1, maxX - minX);
  const sy = h / Math.max(1, maxY - minY);
  const s = Math.min(sx, sy);

  return (x, y) => ({
    x: pad + (x - minX) * s,
    y: canvas.height - (pad + (y - minY) * s)
  });
}

function colorByCell(cell, suspiciousCells) {
  if (suspiciousCells.includes(cell)) return '#ef4444';
  const palette = ['#38bdf8', '#34d399', '#fbbf24', '#c084fc', '#fb7185'];
  return palette[Math.abs(cell) % palette.length];
}

function draw() {
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  if (!model.nodes.length) return;

  const step = model.steps[Math.max(0, Math.min(stepIndex, model.steps.length - 1))] || { suspiciousCells: [], suspiciousNodes: [], t: 0 };
  const tf = fitTransform();

  ctx.strokeStyle = '#334155';
  ctx.lineWidth = 1;
  for (const link of model.links) {
    const a = nodeById(link.a);
    const b = nodeById(link.b);
    if (!a || !b) continue;
    const pa = tf(a.x, a.y);
    const pb = tf(b.x, b.y);
    ctx.beginPath();
    ctx.moveTo(pa.x, pa.y);
    ctx.lineTo(pb.x, pb.y);
    ctx.stroke();
  }

  for (const n of model.nodes) {
    const p = tf(n.x, n.y);
    const inSuspicious = step.suspiciousNodes.includes(n.id);

    ctx.beginPath();
    ctx.fillStyle = colorByCell(n.cell, step.suspiciousCells);
    ctx.arc(p.x, p.y, inSuspicious ? 8 : 6, 0, Math.PI * 2);
    ctx.fill();

    if (n.id === n.leader) {
      ctx.strokeStyle = '#ffffff';
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.arc(p.x, p.y, 11, 0, Math.PI * 2);
      ctx.stroke();
    }

    ctx.fillStyle = '#e2e8f0';
    ctx.font = '11px monospace';
    ctx.fillText(String(n.id), p.x + 8, p.y - 8);
  }

  stepInfoEl.textContent = `Step: ${Math.max(0, Math.min(stepIndex + 1, model.steps.length))}/${model.steps.length} | t=${step.t.toFixed(2)}s`;
  summaryEl.textContent = [
    `nodes=${model.nodes.length}`,
    `links=${model.links.length}`,
    `steps=${model.steps.length}`,
    `suspiciousCells=[${step.suspiciousCells.join(', ')}]`,
    `suspiciousNodes=[${step.suspiciousNodes.join(', ')}]`
  ].join('\n');
}

async function loadLog(path) {
  const res = await fetch(path);
  if (!res.ok) throw new Error(`Cannot load ${path}: ${res.status}`);
  const txt = await res.text();
  model = parseLog(txt);
  stepIndex = 0;
  draw();
}

document.getElementById('loadBtn').addEventListener('click', async () => {
  const path = document.getElementById('logFile').value.trim();
  try {
    await loadLog(path);
  } catch (e) {
    summaryEl.textContent = String(e);
  }
});

document.getElementById('prevBtn').addEventListener('click', () => {
  stepIndex = Math.max(0, stepIndex - 1);
  draw();
});

document.getElementById('nextBtn').addEventListener('click', () => {
  stepIndex = Math.min(Math.max(0, model.steps.length - 1), stepIndex + 1);
  draw();
});

loadLog('sample_network_log.txt').catch(err => {
  summaryEl.textContent = `${err}\n\nTip: run local server in this folder, e.g.\npython3 -m http.server 8080`;
});