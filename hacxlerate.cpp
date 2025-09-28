/*
Frontend - Member 4 (GUI)
File suggestion: src/AgentUI.jsx

This is a single-file React component (JSX) meant to be dropped into a React + Tailwind project
(created with Vite or CRA). It is intentionally self-contained and focuses on UI/UX for the project
requirements you provided.

Dependencies (install if not present):
  - react, react-dom
  - tailwindcss (for styling)
  - framer-motion (animations) -> optional but used here
  - lucide-react (icons) -> optional

Install example:
  npm install framer-motion lucide-react

How to use:
  - Place this file in src/ (e.g. src/AgentUI.jsx)
  - Import it in src/main.jsx / src/App.jsx and render: <AgentUI />
  - Tailwind must be configured in your project for the styles to apply.

Notes on integration:
  - The UI talks to an API at `${backendUrl}/command` and `${backendUrl}/task/:id`.
    These are placeholders. Coordinate with backend/orchestration (Member 3) to finalise schema.
  - The component includes a robust offline/demo fallback so you can demo without a backend.
  - It supports voice (Web Speech API), history (localStorage), CSV/JSON export, filtering, and
    per-task progress monitoring.

If you want a TypeScript version, component-split version, or a ready-to-run Vite template with
Tailwind config, I can generate that next.
*/

import React, { useEffect, useRef, useState } from "react";
import { motion } from "framer-motion";
import { Mic, Send, Download, Trash2 } from "lucide-react";

export default function AgentUI() {
  // --- Persistent settings / state ---
  const [backendUrl, setBackendUrl] = useState(() => localStorage.getItem("agent_backend") || "/api");
  const [command, setCommand] = useState("");
  const [history, setHistory] = useState(() => {
    try {
      return JSON.parse(localStorage.getItem("agent_history") || "[]");
    } catch (e) {
      return [];
    }
  });
  const [results, setResults] = useState([]); // flattened results derived from tasks
  const [isListening, setIsListening] = useState(false);
  const [filterText, setFilterText] = useState("");
  const recognitionRef = useRef(null);
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    localStorage.setItem("agent_history", JSON.stringify(history));
  }, [history]);

  useEffect(() => {
    localStorage.setItem("agent_backend", backendUrl);
  }, [backendUrl]);

  // --- Voice recognition (Web Speech API) ---
  function startVoice() {
    const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
    if (!SpeechRecognition) {
      alert("Voice input not supported in this browser.");
      return;
    }
    const recog = new SpeechRecognition();
    recog.lang = "en-US";
    recog.interimResults = false;
    recog.maxAlternatives = 1;
    recog.onstart = () => setIsListening(true);
    recog.onend = () => setIsListening(false);
    recog.onerror = (e) => {
      console.error("Speech recognition error", e);
      setIsListening(false);
    };
    recog.onresult = (ev) => {
      const text = ev.results[0][0].transcript;
      setCommand((c) => (c ? c + " " + text : text));
    };
    recognitionRef.current = recog;
    recog.start();
  }

  function stopVoice() {
    if (recognitionRef.current) {
      recognitionRef.current.stop();
      recognitionRef.current = null;
    }
    setIsListening(false);
  }

  // --- Helpers for downloads ---
  function downloadJSON(obj, filename = "results.json") {
    const blob = new Blob([JSON.stringify(obj, null, 2)], { type: "application/json" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
  }

  function flattenObject(obj, prefix = "") {
    const out = {};
    for (const key in obj) {
      const val = obj[key];
      const newKey = prefix ? `${prefix}.${key}` : key;
      if (val && typeof val === "object" && !Array.isArray(val)) {
        Object.assign(out, flattenObject(val, newKey));
      } else {
        out[newKey] = Array.isArray(val) ? val.join(" | ") : String(val);
      }
    }
    return out;
  }

  function convertResultsToCSV(items) {
    if (!items || items.length === 0) return "";
    // items: array of arbitrary objects. Flatten and union keys.
    const flat = items.map((it) => flattenObject(it));
    const keys = Array.from(flat.reduce((s, o) => { Object.keys(o).forEach(k => s.add(k)); return s; }, new Set()));
    const lines = [keys.join(",")];
    for (const row of flat) {
      const values = keys.map(k => {
        const v = row[k] !== undefined ? String(row[k]) : "";
        // escape quotes
        return `"${v.replace(/"/g, '""')}"`;
      });
      lines.push(values.join(","));
    }
    return lines.join("\n");
  }

  function downloadCSV(items, filename = "results.csv") {
    const csv = convertResultsToCSV(items);
    const blob = new Blob([csv], { type: "text/csv" });
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    a.click();
    URL.revokeObjectURL(url);
  }

  // --- Core: send command to backend / demo fallback ---
  async function submitCommand(rawText) {
    const text = (rawText || command || "").trim();
    if (!text) return;
    setLoading(true);
    const createdAt = new Date().toISOString();
    const tempId = `local-${Date.now()}`;
    const newHistoryItem = {
      id: tempId,
      command: text,
      status: "queued",
      createdAt,
      progress: 0,
      result: null,
    };
    setHistory((h) => [newHistoryItem, ...h]);
    setCommand("");

    // Try real backend first
    try {
      const res = await fetch(`${backendUrl.replace(/\/+$/, "")}/command`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ command: text }),
      });
      if (!res.ok) throw new Error(`Backend returned ${res.status}`);
      const body = await res.json();
      const taskId = body.task_id || body.id || body.taskId || `task-${Date.now()}`;
      // Replace temp item
      setHistory((h) => h.map(it => it.id === tempId ? { ...it, id: taskId, status: 'running' } : it));
      // Poll
      pollTaskStatus(taskId);
    } catch (err) {
      console.warn("Backend unavailable or failed, using demo fallback", err);
      // Demo fallback (simulate multi-step process)
      simulateDemoTask(tempId, text);
    } finally {
      setLoading(false);
    }
  }

  function simulateDemoTask(tempId, text) {
    // Step simulation with timeouts
    let p = 0;
    setHistory((h) => h.map(it => it.id === tempId ? { ...it, status: 'running' } : it));
    const steps = ["searching web", "extracting results", "ranking & filtering", "finalizing"];
    const interval = setInterval(() => {
      p += 25;
      setHistory((h) => h.map(it => it.id === tempId ? { ...it, progress: Math.min(p, 100) } : it));
      if (p >= 100) {
        clearInterval(interval);
        const demoResults = {
          meta: { source: "demo", query: text, createdAt: new Date().toISOString() },
          items: [
            { title: `Top match for \"${text}\"`, url: "https://example.com/1", price: "₹49,999", score: 0.95 },
            { title: `Second match for \"${text}\"`, url: "https://example.com/2", price: "₹45,999", score: 0.89 },
          ],
        };
        // mark done
        setHistory((h) => h.map(it => it.id === tempId ? { ...it, status: 'done', progress: 100, result: demoResults } : it));
        setResults((r) => [{ id: tempId, command: text, result: demoResults }, ...r]);
      }
    }, 600);
  }

  // Polling for actual backend tasks (Member 3 must provide endpoint '/task/:id')
  function pollTaskStatus(taskId, maxAttempts = 120, intervalMs = 1500) {
    let attempts = 0;
    const handle = setInterval(async () => {
      attempts += 1;
      if (attempts > maxAttempts) {
        clearInterval(handle);
        setHistory((h) => h.map(it => it.id === taskId ? { ...it, status: 'failed' } : it));
        return;
      }
      try {
        const res = await fetch(`${backendUrl.replace(/\/+$/, "")}/task/${taskId}`);
        if (!res.ok) throw new Error("task fetch failed");
        const body = await res.json();
        // body should contain: { status: 'running'|'done'|'failed', progress: 0..100, result }
        setHistory((h) => h.map(it => it.id === taskId ? { ...it, status: body.status || 'running', progress: body.progress ?? it.progress, result: body.result ?? it.result } : it));
        if ((body.status || "").toLowerCase() === "done" || (body.status || "").toLowerCase() === "completed") {
          clearInterval(handle);
          setResults((r) => [{ id: taskId, command: (body.meta && body.meta.query) || '', result: body.result || body.items || body }, ...r]);
        }
      } catch (err) {
        // ignore transient errors; continue polling
        console.debug("poll error (ignored)", err);
      }
    }, intervalMs);
  }

  // --- UI actions ---
  function clearHistory() {
    if (!confirm("Clear local history?")) return;
    setHistory([]);
    setResults([]);
    localStorage.removeItem("agent_history");
  }

  function removeHistoryItem(id) {
    setHistory((h) => h.filter(it => it.id !== id));
    setResults((r) => r.filter(it => it.id !== id));
  }

  function exportAllJSON() {
    downloadJSON({ history, results });
  }

  function exportAllCSV() {
    // Flatten all result.items into rows
    const rows = results.flatMap(r => (r.result && r.result.items) ? r.result.items.map(item => ({ _taskId: r.id, _command: r.command, ...item })) : (r.result ? [{ _taskId: r.id, _command: r.command, raw: JSON.stringify(r.result) }] : []));
    downloadCSV(rows);
  }

  const filteredResults = results.filter(r => {
    if (!filterText) return true;
    const needle = filterText.toLowerCase();
    return (r.command && r.command.toLowerCase().includes(needle)) || (JSON.stringify(r.result || "").toLowerCase().includes(needle));
  });

  // Quick suggestion buttons
  const quickPrompts = [
    { label: "Search laptops under 50k and return top 5", sample: "search for laptops under 50000 INR and return top 5 with prices and links" },
    { label: "Compare two products", sample: "search for iPhone 14 vs Pixel 8 specifications and list differences" },
    { label: "Fill a form", sample: "open example.com contact form and fill name, email, message then submit" },
  ];

  return (
    <div className="min-h-screen bg-gray-50 p-6">
      <div className="max-w-[1200px] mx-auto">
        <header className="mb-6">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-2xl font-bold">Agent GUI — Member 4 (Frontend)</h1>
              <p className="text-sm text-gray-600">Type a command, press Send, and the orchestration will run the plan, control the browser and return structured results.</p>
            </div>
            <div className="flex gap-3 items-center">
              <div className="text-xs text-gray-500">Backend:</div>
              <input
                className="border rounded-md px-2 py-1 text-sm w-48"
                value={backendUrl}
                onChange={(e) => setBackendUrl(e.target.value)}
                title="Backend base URL (e.g. http://localhost:8000)"
              />
              <button className="px-3 py-1 rounded-md bg-white border shadow-sm text-sm" onClick={() => { navigator.clipboard && navigator.clipboard.writeText(backendUrl); alert('Copied backend URL'); }}>Copy</button>
            </div>
          </div>
        </header>

        <main className="grid grid-cols-12 gap-6">
          {/* Left: Command box + quick prompts */}
          <section className="col-span-5">
            <div className="bg-white rounded-2xl shadow p-4">
              <label className="text-sm font-medium">Command</label>
              <textarea
                value={command}
                onChange={(e) => setCommand(e.target.value)}
                placeholder={`Try: ${quickPrompts[0].sample}`}
                className="mt-2 w-full min-h-[120px] border rounded-lg p-3 text-sm resize-y focus:outline-none focus:ring-2 focus:ring-indigo-300"
              />

              <div className="flex items-center gap-2 mt-3">
                <button
                  onClick={() => { isListening ? stopVoice() : startVoice(); }}
                  className={`inline-flex items-center gap-2 px-3 py-2 rounded-xl shadow-sm ${isListening ? 'bg-red-50 border-red-200' : 'bg-white border'} text-sm`}
                >
                  <Mic size={16} />
                  <span>{isListening ? 'Listening...' : 'Voice'}</span>
                </button>

                <button
                  onClick={() => submitCommand()}
                  disabled={loading}
                  className="ml-auto inline-flex items-center gap-2 px-4 py-2 rounded-xl bg-indigo-600 text-white font-medium shadow"
                >
                  <Send size={16} />
                  <span>{loading ? 'Sending...' : 'Send'}</span>
                </button>

                <button onClick={() => { setCommand(''); }} className="px-3 py-2 rounded-xl bg-gray-50 border text-sm">Clear</button>
              </div>

              <div className="mt-4">
                <label className="text-sm font-medium">Quick Prompts</label>
                <div className="mt-2 flex flex-wrap gap-2">
                  {quickPrompts.map((q, i) => (
                    <button key={i} onClick={() => setCommand(q.sample)} className="px-3 py-1 text-xs rounded-full bg-gray-100 border">{q.label}</button>
                  ))}
                </div>
              </div>

              <div className="mt-4 flex items-center justify-between">
                <div className="text-sm text-gray-600">Export</div>
                <div className="flex gap-2">
                  <button onClick={exportAllJSON} className="inline-flex items-center gap-2 px-3 py-1 rounded-lg border"> <Download size={14} /> JSON</button>
                  <button onClick={exportAllCSV} className="inline-flex items-center gap-2 px-3 py-1 rounded-lg border">CSV</button>
                </div>
              </div>
            </div>

            {/* Settings / tips */}
            <div className="mt-4 bg-white rounded-2xl shadow p-4 text-xs text-gray-600">
              <div className="font-medium mb-2">Tips</div>
              <ul className="list-disc pl-5">
                <li>Start with short commands: "search for X" or "compare A vs B".</li>
                <li>Use the backend URL to point to your local orchestration server.</li>
                <li>Voice uses your browser's speech recognition (works best in Chrome).</li>
              </ul>
            </div>
          </section>

          {/* Right: History + results */}
          <section className="col-span-7">
            <div className="flex items-center justify-between mb-3">
              <div className="text-sm text-gray-700 font-medium">Task History</div>
              <div className="flex items-center gap-2">
                <input className="border rounded px-2 py-1 text-sm" placeholder="Filter results..." value={filterText} onChange={(e) => setFilterText(e.target.value)} />
                <button onClick={clearHistory} className="inline-flex items-center gap-2 px-3 py-1 rounded-lg border text-sm"><Trash2 size={14} /> Clear</button>
              </div>
            </div>

            <div className="space-y-3">
              {history.length === 0 && (
                <div className="p-6 text-center text-sm text-gray-500 bg-white rounded-2xl shadow">No history yet — run a command to start.</div>
              )}

              {history.map((h) => (
                <motion.div key={h.id} initial={{ opacity: 0, y: 6 }} animate={{ opacity: 1, y: 0 }} className="bg-white rounded-2xl shadow p-4">
                  <div className="flex items-start gap-3">
                    <div className="flex-1">
                      <div className="flex items-center gap-3">
                        <div className="text-sm font-medium">{h.command}</div>
                        <div className="text-xs text-gray-500">{new Date(h.createdAt || Date.now()).toLocaleString()}</div>
                      </div>

                      <div className="mt-2 text-xs text-gray-600">Status: <span className="font-medium text-gray-800">{h.status || 'queued'}</span></div>

                      <div className="mt-3">
                        <div className="w-full bg-gray-100 rounded-full h-2 overflow-hidden">
                          <div style={{ width: `${h.progress ?? 0}%` }} className="h-2 bg-gradient-to-r from-indigo-400 to-indigo-600" />
                        </div>
                        <div className="text-xs text-gray-500 mt-1">Progress: {h.progress ?? 0}%</div>
                      </div>

                      {h.result && (
                        <div className="mt-3 border rounded p-3 bg-gray-50">
                          <div className="text-sm font-medium mb-2">Result preview</div>
                          <pre className="text-xs whitespace-pre-wrap max-h-36 overflow-auto">{JSON.stringify(h.result, null, 2)}</pre>
                          <div className="mt-2 flex gap-2">
                            <button onClick={() => downloadJSON(h.result, `${h.id}-result.json`)} className="px-3 py-1 rounded bg-white border text-sm inline-flex items-center gap-2">JSON</button>
                            <button onClick={() => {
                              const items = h.result && h.result.items ? h.result.items : [h.result];
                              downloadCSV(items, `${h.id}-result.csv`);
                            }} className="px-3 py-1 rounded bg-white border text-sm">CSV</button>
                            <button onClick={() => { setResults((r) => [{ id: h.id, command: h.command, result: h.result }, ...r]); window.scrollTo({ top: 0, behavior: 'smooth' }); }} className="px-3 py-1 rounded bg-indigo-600 text-white text-sm">Open</button>
                          </div>
                        </div>
                      )}

                    </div>

                    <div className="flex flex-col gap-2">
                      <button onClick={() => removeHistoryItem(h.id)} className="px-3 py-1 rounded bg-red-50 text-red-600 text-xs">Remove</button>
                      <button onClick={() => submitCommand(h.command)} className="px-3 py-1 rounded bg-green-50 text-green-600 text-xs">Rerun</button>
                    </div>
                  </div>
                </motion.div>
              ))}
            </div>

            {/* Results area */}
            <div className="mt-6">
              <div className="flex items-center justify-between mb-3">
                <h2 className="text-lg font-semibold">Results</h2>
                <div className="text-sm text-gray-600">Showing {filteredResults.length} tasks</div>
              </div>

              <div className="space-y-4">
                {filteredResults.map((r) => (
                  <div key={r.id} className="bg-white rounded-2xl shadow p-4">
                    <div className="flex items-center justify-between">
                      <div>
                        <div className="text-sm font-medium">Task: {r.id}</div>
                        <div className="text-xs text-gray-500">Command: {r.command}</div>
                      </div>

                      <div className="flex items-center gap-2">
                        <button onClick={() => downloadJSON(r.result, `${r.id}.json`)} className="px-3 py-1 rounded border inline-flex items-center gap-2">JSON</button>
                        <button onClick={() => {
                          const items = r.result && r.result.items ? r.result.items : [r.result];
                          downloadCSV(items, `${r.id}.csv`);
                        }} className="px-3 py-1 rounded border">CSV</button>
                      </div>
                    </div>

                    <div className="mt-3">
                      {r.result && r.result.items && Array.isArray(r.result.items) ? (
                        <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
                          {r.result.items.map((it, idx) => (
                            <div key={idx} className="border rounded-lg p-3 bg-gray-50">
                              <div className="text-sm font-medium">{it.title || it.name || `Item ${idx+1}`}</div>
                              <div className="text-xs text-gray-600 mt-1">{it.url}</div>
                              <div className="mt-2 text-sm">{it.price ? <span className="font-semibold">{it.price}</span> : null} <span className="ml-2 text-xs text-gray-500">score: {it.score ?? '-'}</span></div>
                            </div>
                          ))}
                        </div>
                      ) : (
                        <pre className="text-xs mt-2 bg-gray-50 p-3 rounded max-h-60 overflow-auto">{JSON.stringify(r.result, null, 2)}</pre>
                      )}
                    </div>
                  </div>
                ))}

                {filteredResults.length === 0 && (
                  <div className="text-center text-gray-500 p-6 bg-white rounded-2xl shadow">No results to show for the filter.</div>
                )}
              </div>
            </div>
          </section>
        </main>

        <footer className="mt-8 text-center text-xs text-gray-500">Frontend prototype • Member 4 — GUI • Tailwind + Framer Motion</footer>
      </div>
    </div>
  );
}
