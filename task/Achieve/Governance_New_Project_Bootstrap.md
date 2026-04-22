# Task: Governance New Project Bootstrap

## Metadata
- Task name: Governance_New_Project_Bootstrap
- Owner: Codex
- Date: 2026-04-21
- Related flow/procedure: master_spec/procedure_spec/New_Project_Governance_Bootstrap.procedure.md
- Task mode: single-task
- Parent master task (for subtask): not_applicable
- Child subtasks (for single independent task): not_applicable
- Acceptance profile (required): A1_Spec_Only
- Acceptance reference (required): master_spec/acceptance_spec/A1_Spec_Only.acceptance.md
- Task review approval (required before implementation): approved
- Current Phase (state machine): Archived
- Last Completed Step: Bootstrap generated startup-required governance instance files
- Next Required Step: none (archived)
- Blocking Condition: none

## Repository Facts To Instantiate
- Repository name: Watchy
- Repository id: watchy
- Repository root: E:\CP\Watchy
- Build command facts: platformio run
- Runtime smoke command facts: platformio run --target upload
- Test entry inventory: Watchy-S3手表/watchy固件源码
- Third-party exclusion paths: .pio/**, lib/**
- Skill registry: bootstrap-governance
- Release pipeline: acceptance-build-and-run

## Background And Goal
- Initialize governance bootstrap for Watchy using repository facts and reusable templates.
- Expected result: the repository has instantiated bootstrap artifacts and a tracked bootstrap record with no startup-readiness gaps.

## Scope
- AGENTS.md
- project_profile.yaml
- master_spec/** governance roots needed by master_spec/initial_spec/initial_spec.md
- task/
- task/Achieve/
- runtime status files such as master_spec/chat_spec/chat_status.md and master_spec/comment_spec/comment_status.md

## Non-Scope
- Algorithm/library code changes
- Release execution
- Repository-specific feature tasks beyond governance initialization

## Plan
1. Confirm bootstrap trigger and repository facts.
2. Create missing governance directories and template-owned files.
3. Instantiate repository-owned files from the appropriate templates.
4. Create runtime/task-state files.
5. Run A1_Spec_Only verification and record the result.

## Execution Update
- Bootstrap input facts were accepted and instantiated into repository-owned governance files.
- Startup-required instance files were generated so the mandatory read order can complete without missing-file gaps.

## Post-Execution Review (required)
- Actual change summary: Generated project_profile.yaml, starter instance catalogs/specs, bootstrap task/manifest, and runtime status files for first-pass repository startup.
- Acceptance command results (exit code): A1 bootstrap initialization verification = 0
- Final acceptance conclusion: pass
- Archive readiness: yes
- Task Conclusion (mandatory):
  - Outcome: accepted
  - Decision: continue
  - Key Evidence: Required governance roots and startup-required instance files were generated.
  - Root Cause Summary: not_applicable
  - Risk Assessment: low
  - Next Action: Review and tailor optional repository-specific details if needed
