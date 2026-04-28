importScripts('https://cdn.jsdelivr.net/npm/@supabase/supabase-js@2');

// IMPORTANT: Replace with your actual Supabase URL and Anon Key
const SUPABASE_URL = 'https://lxeysecudqdlxnssuzml.supabase.co';
const SUPABASE_ANON_KEY = 'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Imx4ZXlzZWN1ZHFkbHhuc3N1em1sIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzczNzM5OTQsImV4cCI6MjA5Mjk0OTk5NH0.lLFUCAUa-sf8gdN8kZm-ucoN575q7k414SUXe-X2SKQ';

let supabase;

if (SUPABASE_URL !== 'YOUR_SUPABASE_URL') {
  // Initialize Supabase from the globally loaded object
  supabase = self.supabase.createClient(SUPABASE_URL, SUPABASE_ANON_KEY);
  
  // 1. Fetch initial data and subscribe to real-time updates
  setupRealtimeTelemetry();
  
  // 2. Setup periodic device status checks (polling every 10 seconds)
  setInterval(checkDeviceStatus, 10000);
} else {
  console.warn("Worker started, but Supabase credentials are missing. Simulating data instead.");
  simulateWorkerData();
}

async function setupRealtimeTelemetry() {
  // ** REPLACE 'telemetry' with your actual table name **
  
  // A) Get initial latest row to prime the dashboard
  const { data, error } = await supabase
    .from('telemetry')
    .select('*')
    .order('created_at', { ascending: false })
    .limit(1);
    
  if (data && data.length > 0) {
    postMessage({ type: 'telemetry', payload: data[0] });
  }

  // B) Subscribe to new inserts in real-time
  const channel = supabase
    .channel('public:telemetry')
    .on(
      'postgres_changes',
      { event: 'INSERT', schema: 'public', table: 'telemetry' },
      (payload) => {
        // Send new data to the main UI thread
        postMessage({ type: 'telemetry', payload: payload.new });
      }
    )
    .subscribe();
}

async function checkDeviceStatus() {
  // ** REPLACE 'devices' with your actual table name for device statuses **
  const { data, error } = await supabase
    .from('devices')
    .select('*');
    
  if (data && !error) {
    // Send updated device statuses to the main UI thread
    postMessage({ type: 'device_status', payload: data });
  }
}

// Fallback simulation logic so the dashboard continues to look alive 
// before you configure your actual Supabase keys.
function simulateWorkerData() {
  let simMicroplastics = 18;
  let simTurbidity = 2.3;
  let simTemp = 26;
  let simTds = 186;
  let simPh = 7.2;
  let simSize = 42;
  let simFlow = 1.8;

  setInterval(() => {
    // Drift values slightly
    simMicroplastics = Math.max(6, Math.min(58, Math.round(simMicroplastics + (Math.random() - 0.45) * 8)));
    simTurbidity = Math.max(0.7, Math.min(9.8, simTurbidity + (Math.random() - 0.5) * 1.2));
    simTemp = Math.max(18, Math.min(35, simTemp + (Math.random() - 0.5) * 2));
    simTds = Math.max(80, Math.min(520, Math.round(simTds + (Math.random() - 0.5) * 36)));
    simPh = Math.max(5.8, Math.min(8.8, simPh + (Math.random() - 0.5) * 0.28));
    simSize = Math.max(18, Math.min(180, Math.round(simSize + (Math.random() - 0.45) * 26)));
    simFlow = Math.max(0.8, Math.min(3.8, simFlow + (Math.random() - 0.5) * 0.28));

    const simData = {
      microplastics: simMicroplastics,
      turbidity: simTurbidity,
      waterTemp: simTemp,
      tds: simTds,
      ph: simPh,
      size: simSize,
      flow: simFlow
    };
    
    postMessage({ type: 'telemetry', payload: simData });
  }, 5200);
}
