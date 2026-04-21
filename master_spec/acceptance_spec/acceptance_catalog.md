# Acceptance Catalog

## Template Binding
- Template skeleton: master_spec/acceptance_spec/ACCEPTANCE_CATALOG_TEMPLATE.md
- Repository classification: repository instance catalog

## Purpose
- Centralize acceptance rules by change type.
- Replace or extend these entries with repository-specific acceptance details when the target repository is ready.

## Profiles
1. A0_Documentation_Only
- File: master_spec/acceptance_spec/A0_Documentation_Only.acceptance.md
- Use for: pure docs/spec/task text updates without code changes.

2. A1_Spec_Only
- File: master_spec/acceptance_spec/A1_Spec_Only.acceptance.md
- Use for: spec-only updates with no code changes.

3. A2_Algorithm_Library_Change
- File: master_spec/acceptance_spec/A2_Algorithm_Library_Change.acceptance.md
- Use for: algorithm or library code changes.

4. A3_Release_Procedure
- File: master_spec/acceptance_spec/A3_Release_Procedure.acceptance.md
- Use for: release/publish procedure execution.
