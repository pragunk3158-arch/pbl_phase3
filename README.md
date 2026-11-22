# SmartWaste - Intelligent Waste Collection and Route Optimization

A C++ project that decides **which dustbins to empty first** and **which path the
truck should take**. It also handles a road getting blocked in the middle of a
trip and finds a new path around it.

There is a console program and an optional web dashboard. All data is stored in
simple text files, no database is used.

---

## 1. How to run

### Windows

```
build.bat          compile the project
run.bat            open the console menu
run_web.bat        open the web dashboard in the browser
```

### Linux or Mac

```
make               compile the project
make run           open the console menu
make test          run the 12 test cases
make demo          run the full demo scenario
make web           open the web dashboard
```

You need **g++** (MinGW on Windows) and, for the dashboard only, **Python 3**.

---

## 2. How a day of collection works

1. **Generate Smart Route** only makes a plan. No waste is picked up yet, so
   pressing it again gives the same bins.
2. **Run All Trips** sends the trucks. Every bin on the plan becomes 0 kg with
   the status `Collected`, and this is saved to `data/bins.txt` straight away.
   One line per truck is added to `data/collections.txt`.
3. **Generate Smart Route** again now plans only the bins that still have waste.
   Critical and high bins (75% and above) go first. When none are left, bins
   of 50% and above are taken, and after that every bin that is not empty.
4. When every bin is empty the page says there is nothing to collect.

Instead of step 2 you can press **Start the Trucks** under the map. Every
truck then drives along its own path in its own colour, stops at each of its
bins with a yellow ring, and that bin is shown as 0 kg at once. The lines under
the map say where every truck is. When all the trucks are back at the depot the
trips are run for real, exactly like Run All Trips, so the bins are saved as
0 kg in the file. Pressing **Stop** before that saves nothing.

**How the truck moves (for the viva).** Every truck keeps `step` (which road it
is on), `progress` (how far along that road, from 0 to 1) and `waiting` (how
long it still stands at a bin). A JavaScript clock (`setInterval`) runs 25
times a second; every time, `progress` grows by 0.05, so one road takes 20
ticks. The point on the screen is the section formula:
`x = x1 + (x2 - x1) * progress`, and the same for `y`. The whole map is drawn
again on every tick, which is what makes the truck look like it is moving.
The path itself comes from the C++ program (Dijkstra); the page only draws it.

A bin with 0 kg is never put on a route, even if its status says `Pending`.
A bin becomes `Pending` again when its waste is updated to more than 0 kg.

---

## 3. Folders

```
include/     header files (class declarations)
src/         .cpp files (class definitions)
data/        text files where the data is saved
web/         the dashboard page (HTML, CSS, JavaScript)
server.py    small Python server that connects the page to the C++ program
```

---

## 4. Data structures used

| Data structure | File | Where it is used |
|---|---|---|
| Singly linked list | `DataStructures.h` (`BinList`) | stores all the bins |
| Max heap / priority queue | `DataStructures.h` (`BinPriorityQueue`) | gives the most urgent bin first |
| Graph (adjacency matrix) | `Graph.h` | the road network |
| Dijkstra's algorithm | `Graph.cpp` | shortest path between two locations |
| Quicksort | `DataStructures.h` | sorting bins by fill level or urgency |
| Linear search | `DataStructures.h` (`findById`) | finding a bin by its ID |

## 5. OOP concepts used

- **Encapsulation** - every class keeps its data private and gives getters and setters.
- **Inheritance** - `CompactorTruck` and `ElectricMiniTruck` extend `Vehicle`.
- **Polymorphism** - `getEffectiveCapacity()` and `getTypeName()` are virtual, so a
  compactor truck reports more capacity than it physically has, while an electric
  mini truck reports its normal capacity.
- **Abstraction** - `WasteManager` hides all the details and gives one function per module.

---

## 6. How the system decides things

**Priority of a bin** is taken from its fill percentage:

| Fill level | Priority |
|---|---|
| 90% and above | CRITICAL |
| 75% to 89% | HIGH |
| 50% to 74% | MEDIUM |
| below 50% | LOW |

**Urgency score** = fill percentage + (fill rate x 1.5), and CRITICAL bins get
50 extra points. The fill rate is included because a bin that fills quickly will
overflow sooner. This score is what the max heap compares.

**Route planning** sends out as many trucks as the waste needs:

1. The priority queue gives the urgent bins, most urgent first.
2. A truck is chosen - the smallest one that can still carry the waiting load.
3. Bins are added to that truck until it is full.
4. Starting from the depot, the truck always goes to the nearest bin it has
   not visited yet (nearest neighbour), and the distance between two stops
   comes from Dijkstra. Then it returns to the depot.
5. If urgent bins are still waiting, steps 2 to 4 run again for the next free
   truck. This repeats until every bin has a truck, or no truck is left.

So the sample data (504 kg urgent) needs only V01, but if every bin is full
(1100 kg) the program sends V03 with 8 bins and V02 with the remaining 2.
In the dashboard each truck gets its own colour on the map.

If a vehicle is named instead of leaving the choice automatic, only that one
truck goes out and the bins that do not fit wait for another day.

**Dynamic re-routing**: when a road is blocked, its edge is marked `blocked` in
the graph. Dijkstra skips blocked edges, so simply running it again produces a
path around the closed road.

**Time estimate** = (distance / 25 km per hour) + 4 minutes for every bin.

---

## 7. Fixed route vs smart route

The fixed route is the traditional method: the truck follows the same order
every single day and stops at every location.

The smart route only visits the bins that are actually urgent, in an order
calculated by Dijkstra.

The program calculates both and prints the difference. The smart route is
shorter, but it is important to say honestly that it does **not** empty the
remaining bins - those are collected on a later trip when they become full.
So the saving is in avoiding unnecessary stops, not in doing the same work in
less distance.

---

## 8. Command line options

The program can also be run without the menu. The web server uses these.

```
SmartWaste --test                    run the 21 test cases
SmartWaste --demo                    run the complete demo scenario
SmartWaste --json                    print the current data as JSON
SmartWaste --api smart V01           plan a route for vehicle V01
SmartWaste --api block A B           block the road between A and B
SmartWaste --api unblock A B         open the road again
SmartWaste --api reroute A B reason  block a road and recalculate the route
SmartWaste --api update B101 95      set the waste of bin B101 to 95 kg
SmartWaste --api addbin B111 Library 100 80
SmartWaste --api collect             run every planned trip
SmartWaste --api compare             compare fixed and smart routes
SmartWaste --api reset               go back to the sample data
```

---

## 9. Data files

| File | What is inside |
|---|---|
| `data/bins.txt` | BinID, Location, Capacity, CurrentWaste, FillRate, Status |
| `data/vehicles.txt` | VehicleID, Type, Capacity, CurrentLoad, Available, Location, Status, Extra |
| `data/roads.txt` | From, To, Distance, Blocked, RoadName |
| `data/collections.txt` | history, one line is added after every trip |
| `data/routes.txt` | the trips planned right now, one per truck (created automatically) |

Lines starting with `#` are comments and are ignored while reading.

If any of these files is missing, the program creates it again with the sample
data of 10 bins, 4 vehicles and 15 roads.

**Why the trips are also saved to a file.** The web dashboard starts the
program again for every button that is pressed, and a new process does not
remember anything from the previous one. So the planned trips are written to
`data/routes.txt`, and the next run reads them back. Without this, pressing
"Run All Trips" would do nothing, because that run would not know that
anything was planned a moment ago. The number of completed trips is counted
from the lines of `collections.txt` for the same reason.

---

## 10. If the dashboard shows nothing

**First check: the header of the page must say "version 3".** If it does not,
the browser is showing an old copy that it saved earlier. Press `Ctrl+F5` once.

The page itself now helps as well:

- A **red box at the top** appears if anything goes wrong, with the reason
  written in it. An empty page with no red box means the files did not load
  at all.
- The server **picks a free port by itself**. If port 5000 is still held by an
  older server window, this one moves to 5001 and the black window prints the
  address it is really using. Always open the address printed in that window.

Other things to check:

- **The program did not compile.** Run `build.bat` on its own and read the
  message. If it says `g++ is not recognized`, MinGW is not installed.
- **Is the back end alive?** Open `/api/state` on the same address the black
  window printed, for example `http://localhost:5000/api/state`. A page full
  of JSON means the C++ side works and only the page is at fault.
- **Close every old black window** before starting a new one, otherwise two
  servers run at the same time and it is easy to look at the wrong one.

---

## 11. Checks the program makes on the data

These checks exist because without them the program either crashed or quietly
produced wrong numbers:

- A line in a data file with a word where a number should be is skipped. It
  used to stop the whole program with a `stod` exception.
- Commas are removed from names that the user types. A location written as
  `Block A, Phase 2` would be saved as two fields and every later run would
  read the file wrongly.
- A bin ID can only be used once, because `findById()` always returns the
  first match and the second bin could never be updated.
- The waste in a bin is never more than its capacity.
- A road from a location to itself, or with a distance of zero or less, is
  refused.
- A bin at a location that has no road to it is still stored, but the program
  warns that no truck can reach it, and its waste is not added to any truck.

---

## 12. Known limits

These are worth knowing before the viva, because they are easy questions to be
asked:

- The route uses **nearest neighbour**, which is a greedy method. It is fast but
  it does not always give the shortest possible tour. A proper solution of the
  travelling salesman problem would be slower.
- The bins are given to the trucks in urgency order until a truck is full. This
  is the greedy "first fit" method. A better split could put fewer kilometres on
  the road, for example by giving one truck all the bins that are close together.
- The graph is kept as an **adjacency matrix**, which is a plain 2D array and is
  easy to read. For a city with many locations and few roads it wastes memory,
  and an adjacency list would be the better choice there.
- Fill levels are typed in by hand. A real system would read them from IoT
  sensors in the bins.
- The time and fuel numbers come from fixed assumptions (25 km/h, 4 minutes per
  bin, 0.28 litres per km). They are estimates, not measured values.
- The whole system runs on one machine for one city, and the data files are
  rewritten completely on every save.
# pbl_phase3
