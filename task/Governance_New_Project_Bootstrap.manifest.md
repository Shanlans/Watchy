# Governance Bootstrap Manifest

## Purpose
- Record the concrete outputs of first-time governance bootstrap for one repository instance.
- Provide one handoff artifact showing what was instantiated, what was verified, and what remains pending.

## Repository Identity
- Repository name: Watchy
- Repository id: watchy
- Repository root: E:\CP\Watchy
- Bootstrap date: 2026-04-21
- Bootstrap owner: Codex

## Automation Summary
- Bootstrap input file used: master_spec/initial_spec/NEW_PROJECT_BOOTSTRAP_INPUT_TEMPLATE.json
- Bootstrap automation entrypoint used: skill/bootstrap-governance/scripts/bootstrap-new-repository.ps1
- Minimum input schema completeness: complete
- Missing required facts at bootstrap time: none

## Instantiated Core Files
- AGENTS.md: AGENTS.md
- project_profile.yaml: project_profile.yaml
- master_spec/master_spec.md: master_spec/master_spec.md
- master_spec/env_spec.md: master_spec/env_spec.md
- master_spec/chat_spec/chat_status.md: master_spec/chat_spec/chat_status.md
- master_spec/comment_spec/comment_status.md: master_spec/comment_spec/comment_status.md
- bootstrap task file under task/: task/Governance_New_Project_Bootstrap.md

## Instantiated Template-Owned Assets
- master_spec/project_profile_template.yaml
- master_spec/initial_spec/NEW_PROJECT_BOOTSTRAP_CHECKLIST.md
- master_spec/initial_spec/NEW_PROJECT_BOOTSTRAP_INPUT_TEMPLATE.json
- master_spec/task_spec/NEW_PROJECT_BOOTSTRAP_TASK_TEMPLATE.md
- any additional copied template-owned files: master_spec/initial_spec/starter_instance/**

## Repository-Specific Facts Confirmed
- Build command facts: platformio run
- Runtime smoke command facts: platformio run --target upload
- Runtime working directory: .
- Test entry inventory: Watchy-S3手表/watchy固件源码
- Third-party exclusion paths: .pio/**, lib/**
- Skill registry: bootstrap-governance
- Release pipeline: acceptance-build-and-run

## Pending Repository-Specific Facts
- facts still unknown at bootstrap time: none
- files intentionally deferred: none
- follow-up owner: repository maintainer

## Verification Summary
- Required structure exists: pass
- Mandatory read order can complete: pass
- Source-repository values removed from instance files: pass
- Notes: Bootstrap produced startup-required instance files and left no mandatory file missing.

## Evidence Links
- project_profile.yaml: project_profile.yaml
- bootstrap task file: task/Governance_New_Project_Bootstrap.md
- master_spec/chat_spec/chat_status.md: master_spec/chat_spec/chat_status.md
- master_spec/comment_spec/comment_status.md: master_spec/comment_spec/comment_status.md
- other bootstrap evidence: task/Governance_New_Project_Bootstrap.manifest.md

## Final Bootstrap Conclusion
- Outcome: accepted
- Risk level: low
- Next action: Review optional repository tailoring items if needed
