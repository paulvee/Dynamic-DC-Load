# Collaborating on Battery Tester

This guide will help you and your collaborator work together on this project using GitHub.

## Repository Information

**GitHub Repository:** https://github.com/paulvee/Battery-tester-Python

## For the Project Owner (You)

### Inviting Your Friend to the Private Repository

1. Go to your repository: https://github.com/paulvee/Battery-tester-Python
2. Click **Settings** (tab at the top)
3. Click **Collaborators** in the left sidebar
4. Click **Add people**
5. Enter your friend's GitHub username or email
6. Click **Add [username] to this repository**
7. They'll receive an invitation email

### Managing the Project

- **Protected Branch:** Consider protecting the `main` branch to require pull requests
- **Issues:** Use GitHub Issues to track bugs and feature requests
- **Projects:** Use GitHub Projects for task management

## For Your Collaborator (Friend)

### Getting Started

1. **Accept the invitation** from the email or GitHub notifications
2. **Clone the repository:**
   ```bash
   git clone https://github.com/paulvee/Battery-tester-Python.git
   cd Battery-tester-Python
   ```

3. **Set up Python environment:**
   ```bash
   python -m venv venv
   venv\Scripts\activate  # Windows
   # or: source venv/bin/activate  # Linux/Mac
   pip install -r requirements.txt
   ```

4. **Test the application:**
   ```bash
   python main.py
   ```

## Git Workflow for Both

### Daily Workflow

1. **Always start by pulling latest changes:**
   ```bash
   git pull origin main
   ```

2. **Create a feature branch for your work:**
   ```bash
   git checkout -b feature/your-feature-name
   # Examples:
   # git checkout -b feature/add-usb-detection
   # git checkout -b bugfix/serial-timeout
   ```

3. **Make your changes and commit frequently:**
   ```bash
   git add .
   git commit -m "Descriptive message about what you changed"
   ```

4. **Push your feature branch:**
   ```bash
   git push origin feature/your-feature-name
   ```

5. **Create a Pull Request (PR) on GitHub:**
   - Go to the repository on GitHub
   - Click "Pull requests" → "New pull request"
   - Select your feature branch
   - Add description of changes
   - Request review from the other person
   - Merge after review

### Keeping Your Branch Updated

If `main` branch gets updated while you're working:

```bash
git checkout main
git pull origin main
git checkout feature/your-feature-name
git merge main
```

## Version Numbering

**IMPORTANT:** Before committing and pushing changes, update the version number:

1. Open `gui/main_window.py`
2. Find the `VERSION` constant (around line 107):
   ```python
   VERSION = "v2.0.a (Python/PyQt6)"
   ```
3. Increment the letter: `a` → `b` → `c` → `d`, etc.
   - Current version: `v2.0.a`
   - Next version: `v2.0.b`
   - After that: `v2.0.c`
4. Save the file and include it in your commit

**Version Scheme:**
- Format: `v2.0.x` where `x` is a letter (a, b, c, d, ...)
- Increment the letter for each push to main
- The letter will reset to `a` when we do a minor version bump (e.g., v2.1.a)

## Communication Best Practices

- **Commit messages:** Be descriptive
  - ✅ Good: "Add voltage calibration dialog with live preview"
  - ❌ Bad: "Fixed stuff" or "Update"

- **Pull Requests:** 
  - Write clear descriptions
  - Reference issues if applicable (#123)
  - Keep PRs focused on one feature/fix

- **Before starting work:**
  - Check existing issues and PRs
  - Communicate what you're working on
  - Avoid duplicate work

## Project Structure

```
Batt Tester/
├── gui/                    # User interface (PyQt6)
├── hardware/              # Serial communication & protocol
├── core/                  # Business logic & data models
├── utils/                 # Configuration & export utilities
├── main.py                # Application entry point
├── test_serial.py         # CLI testing tool
└── requirements.txt       # Dependencies
```

## Testing

Before pushing changes:

1. **Test the application:**
   ```bash
   python main.py
   ```

2. **Test serial communication (if you have hardware):**
   ```bash
   python test_serial.py
   ```

3. **Check for errors:**
   - No Python syntax errors
   - Application launches without crashes
   - Features work as expected

## Common Git Commands

```bash
# Check status
git status

# See what changed
git diff

# View commit history
git log --oneline

# Undo uncommitted changes
git checkout -- filename.py

# Update from GitHub
git pull origin main

# Switch branches
git checkout branch-name

# List all branches
git branch -a

# Delete local branch (after merged)
git branch -d feature/branch-name
```

## Need Help?

- **Git Issues:** https://docs.github.com/en/get-started
- **Python Setup:** See [README.md](README.md)
- **Protocol Details:** See [PROTOCOL_REFERENCE.md](../PROTOCOL_REFERENCE.md)

## Quick Tips

- 📌 **Pull before you push** - Always get latest changes first
- 🔀 **Use branches** - Never work directly on `main`
- 💬 **Communicate** - Let each other know what you're working on
- ✅ **Test** - Run the app before pushing
- 📝 **Document** - Update README/comments as you add features
