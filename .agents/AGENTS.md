# SteamGameServerLauncher AI Customization Rules

## 版本更新與 Commit 要求 (Version Bump & Commit Requirement)
Whenever you (the AI) help the user modify the code and it involves bumping the version number (e.g. modifying `VERSION` in `CMakeLists.txt` or updating `version.h`), you **MUST**:
1. Provide a ready-to-copy `git commit` command for the user in your final response.
2. The commit message MUST include the string `AndBuild` on one of its lines. This triggers the GitHub Actions CI/CD pipeline for the project.

**Example**:
```bash
git add .
git commit -m "Bump version to 1.0.1
AndBuild"
```
Or:
```bash
git add .
git commit -m "Bump version to 1.0.1" -m "AndBuild"
```
Always remind the user to run this command and `git push` it.
