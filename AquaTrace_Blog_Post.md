# Building AquaTrace: The Journey to a Real-Time Water Quality IoT Dashboard

When it comes to environmental monitoring, data is only as useful as the dashboard displaying it. Our goal with the **AquaTrace** project was ambitious but clear: build a secure, real-time IoT telemetry dashboard capable of tracking critical water safety metrics—like microplastic concentration, turbidity, pH, and total dissolved solids (TDS)—streamed directly from edge hardware (like ESP32/ESP8266 microcontrollers).

However, as any developer knows, building a dashboard isn't just about throwing numbers on a screen. It’s about user experience, deployment architecture, and the endless pursuit of eliminating friction.

Here is a deep dive into the engineering journey behind the AquaTrace Dashboard.

---

## Phase 1: From "MechAuth" to "AquaTrace" — The Aesthetic Evolution

The project initially started under the working title "MechAuth," aimed at industrial access systems. However, as the focus shifted specifically to water quality and microplastic tracking, the branding needed an overhaul. 

We transitioned the UI from a rigid industrial aesthetic to a modern, fluid, "glassmorphic" design. The goal was to make the interface feel premium.

### The Challenge of Theming
One of the earliest technical hurdles was implementing a seamless **Light/Dark Mode toggle**. While toggling CSS variables (`var(--bg)`, `var(--panel)`) is straightforward on a single page, keeping that state consistent across multiple pages (`index.html`, `login.html`, `dashboard.html`) without a "theme flash" on navigation is tricky.

**The Solution:**
We implemented a centralized `applyTheme(mode)` Javascript function tied directly to browser `localStorage`.
1. The script checks `localStorage.getItem("theme")` immediately upon page load.
2. It aggressively applies the `data-theme` attribute to the `<html>` root element.
3. We standardized the CSS classes for the toggle button itself (`.t-track`, `.t-thumb`) so that the physical slider button looked identical across all files, fixing a persistent UI glitch where the toggle track background colors would desync.

We also integrated a dynamic HTML5 Canvas background (featuring rotating gears and floating particles) that algorithmically shifted its opacity and stroke colors based on the active theme.

---

## Phase 2: The Authentication Rollercoaster

Initially, the application was bogged down by a traditional, bloated registration system. Users had to fill out massive forms, create passwords, and deal with recovery emails. 

### Attempt 1: Passwordless Magic Links via Supabase
To modernize the flow, we ripped out the entire registration UI. We replaced it with a sleek, single-input field utilizing **Supabase's Passwordless Magic Link** system (`signInWithOtp`). 

Instead of storing passwords, users would simply enter their email, and the Supabase backend would handle secure JWT generation and email delivery.

### The "Silent Failure" Bug
When we deployed the Magic Link system to **Cloudflare Pages**, we hit a wall. Users would click the "Send Access Link" button, and... nothing happened. No error, no green success text, no orange warning text. Just a dead button.

**The Diagnosis:**
Through rigorous debugging and injecting heavy `console.log` trackers, we found the culprit. The button was utilizing an inline HTML event handler: `onclick="handleLogin(event)"`. Inside the Javascript, the first line of the function was `e.preventDefault()`. 

Because we had stripped out the traditional `<form>` tags during our UI cleanup, calling `preventDefault()` on a basic `<div>` button in certain browser environments caused a silent, fatal `TypeError`. The Javascript execution halted completely before it even reached the Supabase API call.

**The Fix:**
We completely refactored the interaction. We removed the inline HTML handlers and migrated to a strict `addEventListener('click', ...)` architecture, wrapped the entire sequence in a robust `try...catch(err)` block to ensure no failure was ever "silent" again.

### Attempt 2: The Strategic Pivot to Open Access
Sometimes, the best engineering decision is realizing you don't need a feature at all. 

Because the dashboard was primarily designed to visualize public/internal sensor telemetry, enforcing an authentication gate was creating unnecessary friction for users just trying to view the data. We made a strategic pivot: **Eliminate the login requirement entirely.**

1. We updated the `index.html` hero button from "LOGIN" to "ENTER DASHBOARD".
2. We removed the `supabase.auth.getSession()` security locks inside `dashboard.html`.
3. We converted the "Logout" buttons into simple "Home" navigation links.

---

## Phase 3: Restoring Database Connectivity

Removing the authentication layer solved our friction problem, but it introduced a new bug.

A core feature of the AquaTrace dashboard is the ability to **Export CSV** files containing historical telemetry data. When users clicked "Export CSV" after our open-access pivot, the application threw a *"Failed to connect or fetch data"* error.

**The Root Cause:**
When we aggressively deleted the Supabase Authentication blocks from `dashboard.html`, we accidentally deleted the `window.supabase.createClient` initialization. The dashboard was attempting to query the `telemetry` table without an active database client.

**The Fix:**
We re-injected the Supabase Data Client initialization block at the top of the script. This restored the connection strictly for data extraction (without enforcing auth protocols). The Javascript now successfully pulls the latest 1,000 telemetry rows, converts the JSON into a CSV blob, and triggers a localized download for the user.

---

## What’s Next: The Edge Hardware

With the web application fully deployed to Cloudflare Pages, flawlessly themed, and securely connected to the Supabase database for historical querying, the frontend architecture is complete.

The final frontier is the physical world. 

Our next milestone is flashing the `iot_device_code.cpp` firmware onto our ESP32 microcontrollers. These devices will interface with physical turbidity, temperature, and TDS sensors, establish a WiFi connection, and begin POSTing live JSON payloads directly to our Supabase REST endpoints.

The foundation is built. It's time to let the data flow.
