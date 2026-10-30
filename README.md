# Smart Clinic Appointment & Patient Management System

A desktop clinic management system written in C++17 with Qt 6 Widgets. The application demonstrates object-oriented design, MVC separation, manual search and sorting algorithms, recursion, STL containers, JSON persistence, validation, and an event-driven GUI.

## Build and run on Windows

Open PowerShell in this project folder. MSYS2 UCRT64, CMake, Ninja, and Qt 6 must be installed under `C:\msys64\ucrt64`.

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
$env:QT_PLUGIN_PATH = "C:\msys64\ucrt64\share\qt6\plugins"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = "C:\msys64\ucrt64\share\qt6\plugins\platforms"
cmake -S . -B build-final -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 -DCMAKE_HAVE_LIBC_PTHREAD=1
cmake --build build-final
.\build-final\scapms.exe
```

In a new Git Bash session:

```bash
cd ~/Documents/MIVA/COS201-LabAssessment
export PATH="/c/msys64/ucrt64/bin:$PATH"
export QT_PLUGIN_PATH="/c/msys64/ucrt64/share/qt6/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="/c/msys64/ucrt64/share/qt6/plugins/platforms"
cmake -S . -B build-final -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 -DCMAKE_HAVE_LIBC_PTHREAD=1
cmake --build build-final
./build-final/scapms.exe
```

The first launch loads `clinic_seed.json` copied beside the executable. Changes are saved in `clinic_data.json` beside the executable. Use **File > New clinic** to create an empty clinic in a new JSON file; the current file is preserved, and the new file becomes the active save target for this session. Use **File > Load JSON** to open another saved clinic file or **File > Save As JSON** to export one. If the window is already open during a rebuild, close it first so Windows releases the executable file.

## Architecture

```text
View: gui/MainWindow
  -> Controllers: ClinicManager, PatientController, AppointmentController,
                  AdminController, SearchEngine, ReportController
  -> Models: PatientRecord hierarchy, Appointment, ClinicDatabase,
             WaitingQueue, AdminActionHistory, PatientAccessCache
  -> Utility: FileHandler, IDGenerator, DateUtils, ClinicException
```

`PatientRecord` is an abstract base class for `GeneralPatient`, `EmergencyPatient`, and `FollowUpPatient`. Each patient category provides its own virtual category label, category detail, and clone operation. `Appointable` defines the appointment operations shared by patient records.

`ClinicDatabase` owns patient objects through `std::unique_ptr` and owns appointment values. Appointments and the queue refer to patients by ID, avoiding duplicate patient objects. Controllers apply the clinic rules and record reversible administrative changes. The Qt view calls controllers in response to buttons, menu actions, keyboard shortcuts, and a timer.

## Required structures and algorithms

- `std::vector<std::unique_ptr<PatientRecord>>` stores polymorphic patients.
- `std::queue<std::string>` provides first-in, first-out consultation calls.
- `std::stack<AdminAction>` supports undo for patient changes, appointment changes, and queue joins/calls.
- `std::array<Entry, 10>` holds the ten most frequently opened records, ranked by their complete access totals. A separate count map retains totals for records outside the current top ten so a later access can move them into the ranking. Searching a result list does not increase this count; opening a patient record does.
- Linear patient ID search checks records one by one.
- Binary patient ID search uses a manually insertion-sorted view, then narrows the range by comparing the middle ID.
- Recursive name search checks one patient per recursive call and stops at the vector length.
- Insertion sort and selection sort are implemented manually and can sort patients by name, ID, age, or next pending appointment date.

## Main screens

- **Dashboard:** clinic totals and today's appointment schedule.
- **Patients:** register, edit, delete, and open patient records. Emergency priority and follow-up notes are category-specific fields.
- **Appointments:** schedule, cancel, and complete appointments.
- **Waiting Queue:** add a patient and call the next patient in FIFO order.
- **Search & Sort:** choose the search method, sort algorithm, and sort field.
- **Reports:** most frequently attended patients, pending appointments, category distribution, today's appointment summary, and most frequently opened records.

The application includes JSON load/save, field validation, exception dialogs, a timer-based appointment reminder, Ctrl+S, Ctrl+Q, Ctrl+F, and Ctrl+Z shortcuts, status messages, and custom category table styles.

The sidebar groups clinic and management navigation, highlights the active page, and uses the bundled clinic logo, care illustration, and six matching SVG icons. When QtSvg is available, the SVG resources are rendered directly; installations without QtSvg use matching vector-drawn icons so the application remains buildable with the base Qt Widgets package.

## Suggested manual walkthrough

1. Open **Dashboard** and confirm that sample patients and today's appointments are shown.
2. Open **Patients**, register `PTTEST1`, `Jordan Lee`, age `28`, contact `0800-000-0000`, category `Emergency`, priority `3`.
3. Select the row, change the name to `Jordan Lee-Smith`, choose **Save edits**, then press **Ctrl+Z** and confirm the original name returns.
4. Open **Appointments**. Use patient ID `PTTEST1`, choose a future date/time, enter `Routine consultation`, and schedule it. Select the appointment and try **Cancel selected**, then press **Ctrl+Z** to restore it.
5. Open **Waiting Queue**, add `PTTEST1`, and call next. Confirm the queue position disappears.
6. Open **Search & Sort**. Try Linear by ID with `PT0001`, Binary by ID with `PT0008`, Recursive by name with `ada`, and Category with `Emergency`. Confirm search result lists do not affect the opened-record report.
7. Sort by `Patient name` using Insertion sort, then by `Age` using Selection sort.
8. Double-click a patient row to open the patient record. Repeat with two records, then open **Reports** and inspect Frequently opened records.
9. Use **File > Save**, close the app, and reopen it. Confirm the record changes persist. Use **File > Load JSON** with the sample file to restore the original sample data if desired.

## Assessment coverage

| Assessment area | Implementation |
| --- | --- |
| Abstract patient class and three subclasses | `model/PatientRecord.h`, `model/GeneralPatient.h`, `model/EmergencyPatient.h`, `model/FollowUpPatient.h` |
| Interface and polymorphism | `model/Appointable.h`; controller and view consume `PatientRecord` base references |
| MVC/modules/namespaces | `model/`, `controller/`, `gui/`, `utils/`; namespace `scapms` |
| Required containers | Vector, queue, stack, and fixed array as described above |
| Searching and recursion | `controller/SearchEngine.h` |
| Two manual sorts and selectable fields | `SearchEngine` insertion/selection implementations; Search & Sort panel |
| GUI, layouts, events, validation, timer, dialogs, file picker, shortcuts | `gui/MainWindow.cpp` |
| Persistence and reports | `utils/FileHandler.h`, `controller/ReportController.h` |

The assessment's final submission also calls for screenshots, a UML diagram, source listings, and a short report. The app and this README provide the implementation and a manual walkthrough; capture the requested screenshots from the running program for the course submission.
