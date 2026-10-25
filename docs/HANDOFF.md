# Handoff log

Dated entries, newest first. See AGENTS.md for the rules and PROGRESS.md for the full run log.

## 2026-10-25 - Run 95 (Grok Bot), branch `run95`

- New toggle `wide_early=1` (Debug tab "Wide layout early"): with `wide_layouts=1` the wide compositor runs while engine B renders
  and the redundant melonDS engine A pass is skipped. Identical output; menus/title about 40% less composite time in Azahar.
- New log lines: `[PERF4]` (game-thread V-blank wait), a second `[SPIKE]` stage line, `[WIDE] wide_early` and `[WIDE] parts set` (diag).
- Version string run95. Open: PC boxes / battle / shiny_suite / text_auto not emulator-tested this run; dirty-row widecomp is the
  next speedup; gpu_compose=2 readback skip not attempted.
