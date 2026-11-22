// SmartWaste dashboard.
// The page never calculates anything by itself. It asks the C++ program
// through the Python server and only shows what comes back.

var state = null;

// For the moving trucks (see "MOVING TRUCKS ON THE MAP" further down)
var movingTrucks = [];     // one entry for every truck that is driving
var timer = null;          // the clock that moves them, null when stopped

// One colour for each truck, so the trips can be told apart on the map
var routeColours = ["#10b981", "#f59e0b", "#a78bfa", "#38bdf8", "#f472b6"];

function routeColour(index) {
  return routeColours[index % routeColours.length];
}

// Where each location is drawn on the canvas
var positions = {
  "Depot":            { x: 100, y: 240 },
  "GEU Gate":         { x: 230, y: 120 },
  "Market":           { x: 230, y: 360 },
  "Parking":          { x: 250, y: 240 },
  "Hostel":           { x: 380, y: 100 },
  "Main Road":        { x: 380, y: 380 },
  "Library":          { x: 420, y: 220 },
  "Canteen":          { x: 530, y: 110 },
  "Academic Block":   { x: 610, y: 230 },
  "Sports Complex":   { x: 700, y: 120 },
  "Residential Area": { x: 680, y: 370 }
};

// ---------------------------------------------------------------
// Talking to the server
// ---------------------------------------------------------------

// Shows a big red box at the top of the page
function showBanner(text) {
  var box = document.getElementById("errorBanner");
  if (box === null) return;
  box.style.display = "block";
  box.textContent = text;
}

function hideBanner() {
  var box = document.getElementById("errorBanner");
  if (box !== null) box.style.display = "none";
}

function loadState() {
  fetch("/api/state")
    .then(function (response) { return response.json(); })
    .then(function (data) {
      if (data.error) {
        showBanner(data.error);
        return;
      }
      hideBanner();
      state = data;
      showEverything();
    })
    .catch(function (error) {
      showBanner("Cannot reach the server. Is the black window with " +
                 "server.py still open? (" + error + ")");
    });
}

// Shows a short line of text under the route box
function showMessage(text, isError) {
  var box = document.getElementById("message");
  box.textContent = text;
  box.className = isError ? "msg error" : "msg";
}

// Sends one action and shows the new state that comes back
function sendAction(action, options, message) {
  // While the trucks are driving, the plan on the screen must not change
  if (timer !== null) {
    showMessage("The trucks are still driving. Wait for them, or press Stop.", true);
    return;
  }

  var query = "action=" + action;
  for (var key in options) {
    query = query + "&" + key + "=" + encodeURIComponent(options[key]);
  }

  fetch("/api/action?" + query)
    .then(function (response) { return response.json(); })
    .then(function (data) {
      // the server sends {"error": "..."} when the C++ program failed
      if (data.error) {
        showMessage(data.error, true);
        return;
      }
      state = data;
      showEverything();

      if (action === "smart" && state.routes.length === 0)
        showMessage("Every bin is empty, there is nothing to collect.", false);
      else if (message)
        showMessage(message, false);
    })
    .catch(function (error) {
      console.log("Action failed:", error);
      showMessage("The server did not answer. Is server.py still running?", true);
    });
}

// True if at least one trip has been planned
function hasRoutes() {
  return state !== null && state.routes && state.routes.length > 0;
}

// ---------------------------------------------------------------
// Buttons
// ---------------------------------------------------------------

function planSmartRoute() {
  var vehicle = document.getElementById("vehicleSelect").value;
  var name = (vehicle === "") ? "all the trucks that are needed" : "truck " + vehicle;

  // Planning again before running the trips gives the same bins, because
  // nothing has been collected yet. Say so, otherwise it looks like a bug.
  var message;
  if (hasRoutes())
    message = "Planned again using " + name + ". The bins are the same as before " +
              "because nothing was collected yet. Press Run All Trips to collect them.";
  else
    message = "Trips planned using " + name + ". Now press Run All Trips to collect the waste.";

  sendAction("smart", { vehicle: vehicle }, message);
}

// Every bin that is on today's plan, from all the trucks together
function binsOnPlan() {
  var list = [];
  for (var i = 0; i < state.routes.length; i++) {
    for (var j = 0; j < state.routes[i].bins.length; j++)
      list.push(state.routes[i].bins[j]);
  }
  return list;
}

function collectWaste() {
  if (!hasRoutes()) {
    showMessage("No trips planned yet. Press Generate Smart Route first.", true);
    return;
  }

  sendAction("collect", {}, "Done. Emptied " + binsOnPlan().join(", ") +
             " and saved them as 0 kg. Press Generate Smart Route " +
             "to plan the bins that are still full.");
}

function resetData() {
  sendAction("reset", {}, "Sample data restored.");
}

function runComparison() {
  sendAction("compare", {}, "Comparison calculated by the C++ program.");
}

function blockRoad() {
  var from = document.getElementById("fromSelect").value;
  var to = document.getElementById("toSelect").value;
  if (from === to) {
    alert("Please choose two different locations.");
    return;
  }
  if (!hasRoutes()) {
    showMessage("Plan the trips first, then block a road to see the re-routing.", true);
    return;
  }
  sendAction("reroute", { from: from, to: to, reason: "Road repair" },
             "Road closed. The route was calculated again.");
}

function unblockRoad() {
  var from = document.getElementById("fromSelect").value;
  var to = document.getElementById("toSelect").value;
  sendAction("unblock", { from: from, to: to }, "Road is open again.");
}

// Returns the number, or null if the text is not a valid positive number
function readNumber(text) {
  if (text === null || text.trim() === "") return null;
  var value = Number(text);
  if (isNaN(value) || value < 0) return null;
  return value;
}

function updateBin(id) {
  var waste = readNumber(prompt("New waste amount in kg for bin " + id + ":"));
  if (waste === null) {
    showMessage("That is not a valid amount, nothing was changed.", true);
    return;
  }
  sendAction("update", { binId: id, waste: waste }, "Bin " + id + " updated.");
}

function addBin() {
  var id = prompt("Bin ID (example B111):");
  if (!id) return;
  id = id.trim();

  // the C++ side refuses a repeated ID, this just says so immediately
  for (var i = 0; i < state.bins.length; i++) {
    if (state.bins[i].id === id) {
      showMessage("A bin with the ID " + id + " already exists.", true);
      return;
    }
  }

  var location = prompt("Location (example Library):");
  if (!location) return;
  location = location.trim();

  if (knownLocations().indexOf(location) === -1) {
    showMessage("\"" + location + "\" is not on the road map, so no truck " +
                "can reach it. Use one of the locations in the dropdown lists.", true);
    return;
  }

  var capacity = readNumber(prompt("Capacity in kg:", "100"));
  var waste = readNumber(prompt("Current waste in kg:", "80"));
  if (capacity === null || capacity <= 0 || waste === null) {
    showMessage("Capacity and waste must be numbers, nothing was added.", true);
    return;
  }

  sendAction("addbin", { id: id, location: location, capacity: capacity, waste: waste },
             "Bin " + id + " added.");
}

// Plans a route and then blocks a road, so the whole story is visible
function runDemo() {
  planSmartRoute();
  setTimeout(function () {
    sendAction("reroute", { from: "Hostel", to: "Canteen", reason: "Road repair" },
               "Hostel - Canteen closed, a longer path through Library is used.");
  }, 1200);
}

function runTests() {
  var box = document.getElementById("testOutput");
  box.textContent = "Running...";
  fetch("/api/test")
    .then(function (response) { return response.text(); })
    .then(function (text) { box.textContent = text; })
    .catch(function () { box.textContent = "Could not run the tests."; });
}

// ---------------------------------------------------------------
// Drawing the page
// ---------------------------------------------------------------

function showEverything() {
  if (state === null) return;

  // new data from the server means the old trucks are not needed any more
  stopTrucks();
  movingTrucks = [];
  showSummary();
  showBins();
  showVehicles();
  showRoutes();
  showComparison();
  fillSelects();
  fillVehicleSelect();
  drawGraph();
}

function setText(id, text) {
  document.getElementById(id).textContent = text;
}

function showSummary() {
  var m = state.metrics;
  setText("totalBins", m.totalBins);
  setText("pendingWaste", "Pending: " + m.pendingWaste.toFixed(0) + " kg");
  setText("criticalBins", m.criticalBins);
  setText("highBins", "High priority: " + m.highBins);
  setText("availableVehicles", m.availableVehicles + " of " + state.vehicles.length);

  // how many trucks the current plan sends out
  var trucks = state.routeTotals.trucks;
  if (trucks === 0)
    setText("trucksSent", "No trip planned yet");
  else if (trucks === 1)
    setText("trucksSent", "1 truck is sent out today");
  else
    setText("trucksSent", trucks + " trucks are sent out today");
  setText("completedTrips", m.completedTrips);
}

function showBins() {
  var body = document.querySelector("#binsTable tbody");
  body.innerHTML = "";

  for (var i = 0; i < state.bins.length; i++) {
    var bin = state.bins[i];
    var style = bin.priority.toLowerCase();   // critical, high, medium, low

    var row = document.createElement("tr");
    if (bin.status === "Collected") row.className = "collected-row";
    row.innerHTML =
      "<td><strong>" + bin.id + "</strong></td>" +
      "<td>" + bin.location + "</td>" +
      "<td>" + bin.capacity + " kg</td>" +
      "<td>" + bin.currentWaste.toFixed(1) + " kg</td>" +
      "<td><span class='bar'><span class='fill-" + style +
         "' style='width:" + Math.min(bin.fillPercent, 100) + "%'></span></span>" +
         bin.fillPercent.toFixed(1) + "%</td>" +
      "<td><span class='tag tag-" + style + "'>" + bin.priority + "</span></td>" +
      "<td>" + bin.status + "</td>" +
      "<td><button class='btn' onclick=\"updateBin('" + bin.id + "')\">Update</button></td>";
    body.appendChild(row);
  }
}

function showVehicles() {
  var body = document.querySelector("#vehiclesTable tbody");
  body.innerHTML = "";

  for (var i = 0; i < state.vehicles.length; i++) {
    var v = state.vehicles[i];
    var row = document.createElement("tr");
    row.innerHTML =
      "<td><strong>" + v.id + "</strong></td>" +
      "<td>" + v.type + "</td>" +
      "<td>" + v.capacity + " kg</td>" +
      "<td><strong>" + v.effectiveCapacity + " kg</strong></td>" +
      "<td>" + v.currentLoad.toFixed(1) + " kg</td>" +
      "<td class='" + (v.available ? "green" : "red") + "'>" +
         (v.available ? "Yes" : "No") + "</td>" +
      "<td>" + v.location + "</td>" +
      "<td>" + v.status + "</td>";
    body.appendChild(row);
  }
}

function showRoutes() {
  var box = document.getElementById("routeList");
  var heading = document.getElementById("tripsHeading");
  box.innerHTML = "";

  if (state.routes.length === 0) {
    heading.textContent = "Planned Trips";
    box.innerHTML = "<p class='help'>No trips planned yet. Press Generate Smart Route.</p>";
    return;
  }

  var word = (state.routes.length === 1) ? " truck" : " trucks";
  heading.textContent = "Planned Trips - " + state.routes.length + word;

  // one card for each truck
  for (var i = 0; i < state.routes.length; i++) {
    var route = state.routes[i];

    var card = document.createElement("div");
    card.className = "route-box";

    var text =
      "<p class='route-title'><span class='route-colour' style='background:" +
        routeColour(i) + "'></span>Truck " + route.vehicleId + "</p>" +
      "<p><span>Distance</span> <strong>" + route.totalDistance.toFixed(2) + " km</strong></p>" +
      "<p><span>Time</span> <strong>" + route.estimatedTime.toFixed(1) + " mins</strong></p>" +
      "<p><span>Load</span> <strong>" + route.totalWaste.toFixed(1) + " kg</strong></p>" +
      "<p><span>Bins</span> <strong>" + route.bins.length + "</strong></p>";

    if (route.reRouted) {
      text += "<p class='status warning'>Re-routed: " + route.reRouteReason + "</p>";
    } else {
      text += "<p class='status'>Normal optimized route</p>";
    }

    text += "<p class='path'>" + route.path.join("  ->  ") + "</p>";
    card.innerHTML = text;
    box.appendChild(card);
  }

  // one line with the total of all the trucks together
  var totals = state.routeTotals;
  var summary = document.createElement("p");
  summary.className = "route-total";
  var trucksWord = (totals.trucks === 1) ? " truck sent out" : " trucks sent out";
  summary.textContent = totals.trucks + trucksWord + "  |  " +
                        totals.distance.toFixed(2) + " km in total  |  " +
                        totals.waste.toFixed(1) + " kg of waste";
  box.appendChild(summary);
}

function showComparison() {
  var c = state.comparison;
  if (!c) return;                 // comparison was not run in this request

  setText("cmpFixedDist",  c.fixedDistance.toFixed(1) + " km");
  setText("cmpSmartDist",  c.smartDistance.toFixed(1) + " km");
  setText("cmpFixedTime",  c.fixedTime.toFixed(0) + " mins");
  setText("cmpSmartTime",  c.smartTime.toFixed(0) + " mins");
  setText("cmpFixedBins",  c.fixedBins + " bins");
  setText("cmpSmartBins",  c.smartBins + " bins");
  setText("cmpFixedWaste", c.fixedWaste.toFixed(0) + " kg");
  setText("cmpSmartWaste", c.smartWaste.toFixed(0) + " kg");
  setText("cmpSmartTrucks", c.smartTrucks);

  setText("savedDistance", c.distanceSaved.toFixed(1) + " km");
  setText("savedTime",     c.timeSaved.toFixed(0) + " min");
  setText("savedFuel",     c.fuelSaved.toFixed(2) + " L");
  setText("savedPercent",  c.efficiencyGain.toFixed(1) + " %");
}

// Every location that really appears in the road list, sorted by name.
// Reading them from the data means a road added later also shows up here.
function knownLocations() {
  var names = [];
  for (var i = 0; i < state.roads.length; i++) {
    if (names.indexOf(state.roads[i].from) === -1) names.push(state.roads[i].from);
    if (names.indexOf(state.roads[i].to) === -1) names.push(state.roads[i].to);
  }
  names.sort();
  return names;
}

// Fills the two location lists, only the first time
function fillSelects() {
  var fromSelect = document.getElementById("fromSelect");
  var toSelect = document.getElementById("toSelect");
  if (fromSelect.options.length > 0) return;

  var names = knownLocations();
  for (var i = 0; i < names.length; i++) {
    fromSelect.appendChild(new Option(names[i], names[i]));
    toSelect.appendChild(new Option(names[i], names[i]));
  }
  fromSelect.value = "Hostel";
  toSelect.value = "Canteen";
}

// Fills the vehicle list, only the first time
function fillVehicleSelect() {
  var select = document.getElementById("vehicleSelect");
  if (select.options.length > 0) return;

  select.appendChild(new Option("Use as many trucks as needed", ""));
  for (var i = 0; i < state.vehicles.length; i++) {
    var v = state.vehicles[i];
    select.appendChild(new Option("Only " + v.id + " - " + v.type, v.id));
  }
  select.value = "";
}

// Sorts the table only. The real sorting in C++ is in the console menu.
function sortBins(type) {
  if (state === null) return;

  if (type === "fill") {
    state.bins.sort(function (a, b) { return b.fillPercent - a.fillPercent; });
  } else {
    var order = { "CRITICAL": 4, "HIGH": 3, "MEDIUM": 2, "LOW": 1 };
    state.bins.sort(function (a, b) { return order[b.priority] - order[a.priority]; });
  }
  showBins();
}

function showTab(pageId, button) {
  var pages = document.querySelectorAll(".tab-page");
  for (var i = 0; i < pages.length; i++) pages[i].classList.remove("active");

  var tabs = document.querySelectorAll(".tab");
  for (var j = 0; j < tabs.length; j++) tabs[j].classList.remove("active");

  document.getElementById(pageId).classList.add("active");
  button.classList.add("active");

  if (pageId === "tab-map") drawGraph();
}

// ---------------------------------------------------------------
// Canvas drawing of the graph
// ---------------------------------------------------------------

// Which truck drives on the road from-to. Returns the route number,
// or -1 if no planned trip uses this road.
function whichRouteUses(from, to) {
  for (var r = 0; r < state.routes.length; r++) {
    var path = state.routes[r].path;
    for (var i = 0; i + 1 < path.length; i++) {
      if ((path[i] === from && path[i + 1] === to) ||
          (path[i] === to && path[i + 1] === from)) {
        return r;
      }
    }
  }
  return -1;
}

function colourOfLocation(name) {
  if (name === "Depot") return "#38bdf8";

  for (var i = 0; i < state.bins.length; i++) {
    if (state.bins[i].location !== name) continue;
    var priority = state.bins[i].priority;
    if (priority === "CRITICAL") return "#ef4444";
    if (priority === "HIGH") return "#f59e0b";
    if (priority === "MEDIUM") return "#38bdf8";
    return "#10b981";
  }
  return "#64748b";
}

function drawGraph() {
  var canvas = document.getElementById("graphCanvas");
  if (!canvas || state === null) return;

  var pen = canvas.getContext("2d");
  pen.clearRect(0, 0, canvas.width, canvas.height);

  // roads first, so the circles are drawn on top of them
  for (var i = 0; i < state.roads.length; i++) {
    var road = state.roads[i];
    var a = positions[road.from];
    var b = positions[road.to];
    if (!a || !b) continue;

    var usedBy = whichRouteUses(road.from, road.to);

    pen.beginPath();
    pen.moveTo(a.x, a.y);
    pen.lineTo(b.x, b.y);

    if (road.blocked) {
      pen.strokeStyle = "#ef4444";
      pen.lineWidth = 4;
      pen.setLineDash([6, 6]);
    } else if (usedBy >= 0) {
      pen.strokeStyle = routeColour(usedBy);
      pen.lineWidth = 5;
      pen.setLineDash([]);
    } else {
      pen.strokeStyle = "#334155";
      pen.lineWidth = 2;
      pen.setLineDash([]);
    }
    pen.stroke();
    pen.setLineDash([]);

    // distance written in the middle of the road
    var midX = (a.x + b.x) / 2;
    var midY = (a.y + b.y) / 2;
    pen.textAlign = "center";
    pen.font = "10px Arial";
    pen.fillStyle = road.blocked ? "#f87171" : (usedBy >= 0 ? routeColour(usedBy) : "#64748b");
    pen.fillText(road.distance + " km", midX, midY - 4);

    if (road.blocked) {
      pen.fillStyle = "#ef4444";
      pen.font = "bold 11px Arial";
      pen.fillText("BLOCKED", midX, midY + 12);
    }
  }

  // locations
  for (var name in positions) {
    var place = positions[name];
    var radius = (name === "Depot") ? 20 : 16;

    pen.beginPath();
    pen.arc(place.x, place.y, radius, 0, Math.PI * 2);
    pen.fillStyle = colourOfLocation(name);
    pen.fill();
    pen.strokeStyle = "#0f172a";
    pen.lineWidth = 2;
    pen.stroke();

    pen.fillStyle = "#ffffff";
    pen.font = "bold 11px Arial";
    pen.textAlign = "center";
    pen.fillText(name, place.x, place.y + radius + 14);

    // fill level under the name
    for (var k = 0; k < state.bins.length; k++) {
      if (state.bins[k].location === name) {
        pen.fillStyle = "#94a3b8";
        pen.font = "10px Arial";
        pen.fillText(state.bins[k].fillPercent.toFixed(0) + "% (" + state.bins[k].id + ")",
                     place.x, place.y + radius + 26);
        break;
      }
    }
  }

  // the moving trucks go on top of everything else
  drawTrucks(pen);
}

// ===========================================================================
// MOVING TRUCKS ON THE MAP
//
// Every truck keeps five small values:
//   step      - which road of its path it is driving on
//   progress  - how far along that road: 0 = at the start, 1 = at the end
//   waiting   - how many ticks it still stands at a bin
//   collected - the bins it has already emptied
//   finished  - true when it is back at the depot
//
// A clock calls moveTrucks() 25 times every second. Each call moves every
// truck a little, and draws the whole map again. Drawing it again and again
// very fast is what makes the truck look like it is moving.
//
// When a truck stops at one of its bins, that bin is shown as 0 kg at once.
// When all the trucks are back, the C++ program is asked to run the trips,
// which empties the bins in the file and writes the history. So the result
// is exactly the same as pressing Run All Trips.
// ===========================================================================

var STEP_SIZE = 0.05;      // part of a road covered in one tick (20 ticks per road)
var STOP_TICKS = 16;       // how many ticks a truck stands at a bin

function playTrucks() {
  if (timer !== null) return;               // already driving

  if (!hasRoutes()) {
    showMessage("Plan the trips first: press Generate Smart Route.", true);
    return;
  }

  movingTrucks = [];
  for (var i = 0; i < state.routes.length; i++) {
    movingTrucks.push({
      route: i,
      step: 0,
      progress: 0,
      waiting: 0,
      collected: [],
      finished: false
    });
  }

  timer = setInterval(moveTrucks, 40);      // 40 ms = 25 times a second
  showMessage("The trucks have left the depot.", false);
}

// Only stops the clock. Used inside the program.
function stopTrucks() {
  if (timer !== null) {
    clearInterval(timer);
    timer = null;
  }
}

// The Stop button. The trips did not finish, so nothing is saved and the
// bins that were shown as empty on the screen are loaded again from the file.
function cancelTrucks() {
  if (timer === null) return;

  stopTrucks();
  movingTrucks = [];
  showMessage("Stopped. Nothing was saved, because the trucks did not finish.", true);
  document.getElementById("truckStatus").textContent = "Stopped.";
  loadState();
}

// Moves every truck a little bit and draws the map again
function moveTrucks() {
  var allFinished = true;

  for (var i = 0; i < movingTrucks.length; i++) {
    var truck = movingTrucks[i];
    var path = state.routes[truck.route].path;

    if (truck.finished) continue;
    allFinished = false;

    // 1. standing at a bin
    if (truck.waiting > 0) {
      truck.waiting = truck.waiting - 1;
      continue;
    }

    // 2. driving along the road
    truck.progress = truck.progress + STEP_SIZE;
    if (truck.progress < 1) continue;

    // 3. reached the next location
    truck.progress = 0;
    truck.step = truck.step + 1;

    if (truck.step >= path.length - 1) {
      truck.finished = true;                // back at the depot
      continue;
    }

    // 4. one of its bins is here, so stop and empty it
    var binId = binAtLocation(truck, path[truck.step]);
    if (binId !== "") {
      truck.collected.push(binId);
      truck.waiting = STOP_TICKS;
      emptyBinOnScreen(binId);
    }
  }

  drawGraph();
  showTruckStatus();

  if (allFinished) finishTrips();
}

// All the trucks are back: ask the C++ program to run the trips for real
function finishTrips() {
  var emptied = binsOnPlan();
  stopTrucks();     // must be stopped first, otherwise sendAction refuses

  sendAction("collect", {}, "All trucks are back. Emptied " + emptied.join(", ") +
             " and saved them as 0 kg. Press Generate Smart Route " +
             "to plan the bins that are still full.");
}

// Returns the bin this truck still has to empty at this place, or ""
function binAtLocation(truck, place) {
  var route = state.routes[truck.route];

  for (var i = 0; i < state.bins.length; i++) {
    var bin = state.bins[i];
    if (bin.location !== place) continue;
    if (route.bins.indexOf(bin.id) === -1) continue;        // not this truck's bin
    if (truck.collected.indexOf(bin.id) !== -1) continue;   // already emptied
    return bin.id;
  }
  return "";
}

// Shows a bin as empty on the screen the moment the truck stops there.
// This only changes what is shown. The file is changed by finishTrips().
function emptyBinOnScreen(binId) {
  for (var i = 0; i < state.bins.length; i++) {
    if (state.bins[i].id === binId) {
      state.bins[i].currentWaste = 0;
      state.bins[i].fillPercent = 0;
      state.bins[i].priority = "LOW";
      state.bins[i].status = "Collected";
    }
  }
  showBins();
}

// Where the truck is right now, as x and y on the canvas.
// This is the section formula: the point that divides the road from
// "from" to "to" in the ratio progress : (1 - progress).
function truckPosition(truck) {
  var path = state.routes[truck.route].path;
  var from = positions[path[truck.step]];
  var to = positions[path[truck.step + 1]];
  if (!from) return null;
  if (!to) return from;

  return {
    x: from.x + (to.x - from.x) * truck.progress,
    y: from.y + (to.y - from.y) * truck.progress
  };
}

function drawTrucks(pen) {
  for (var i = 0; i < movingTrucks.length; i++) {
    var truck = movingTrucks[i];
    if (truck.finished) continue;

    var place = truckPosition(truck);
    if (place === null) continue;

    // a small box in the truck's colour with its number on it
    pen.fillStyle = routeColour(truck.route);
    pen.strokeStyle = "#0f172a";
    pen.lineWidth = 2;
    pen.fillRect(place.x - 16, place.y - 11, 32, 22);
    pen.strokeRect(place.x - 16, place.y - 11, 32, 22);

    pen.fillStyle = "#0f172a";
    pen.font = "bold 11px Arial";
    pen.textAlign = "center";
    pen.fillText(state.routes[truck.route].vehicleId, place.x, place.y + 4);

    // a yellow ring while it is emptying a bin
    if (truck.waiting > 0) {
      pen.beginPath();
      pen.arc(place.x, place.y, 24, 0, Math.PI * 2);
      pen.strokeStyle = "#fbbf24";
      pen.lineWidth = 3;
      pen.stroke();
    }
  }
}

// One line for every truck saying where it is
function showTruckStatus() {
  var lines = [];

  for (var i = 0; i < movingTrucks.length; i++) {
    var truck = movingTrucks[i];
    var route = state.routes[truck.route];
    var path = route.path;
    var name = route.vehicleId;
    var done = truck.collected.length + " of " + route.bins.length + " bins done";

    if (truck.finished) {
      lines.push(name + ": back at the depot (" + done + ")");
    } else if (truck.waiting > 0) {
      var bin = truck.collected[truck.collected.length - 1];
      lines.push(name + ": emptying bin " + bin + " at " + path[truck.step] + " (" + done + ")");
    } else {
      lines.push(name + ": driving from " + path[truck.step] + " to " +
                 path[truck.step + 1] + " (" + done + ")");
    }
  }

  document.getElementById("truckStatus").textContent = lines.join("\n");
}

// start
loadState();
