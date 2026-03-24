# How to Create GitHub Release

Follow these steps to create your first GitHub Release:

## Option A: Via GitHub Website (Recommended)

### 1. Navigate to Releases Page

Go to: https://github.com/paulvee/Battery-tester-Python/releases

Or:
1. Open your repository: https://github.com/paulvee/Battery-tester-Python
2. Look at the right sidebar
3. Click on "Releases" (might show "0 releases")
4. If you don't see it, click on the "Code" tab first, then look on the right side

### 2. Create New Release

Click the **"Create a new release"** or **"Draft a new release"** button

### 3. Fill in Release Details

**Tag version:** (Required)
```
v2.0.1
```
- Click "Choose a tag" dropdown
- Type `v2.0.1` (must start with 'v')
- Click "Create new tag: v2.0.1 on publish"

**Release title:** (Optional but recommended)
```
Battery Tester v2.0.1 - Windows Release
```

**Description:** (Copy the content from RELEASE_NOTES.md)
- Open `RELEASE_NOTES.md` in VS Code
- Copy all the content
- Paste into the description box

### 4. Attach Binary File

**Scroll down to "Attach binaries"** section:
1. Click to open file picker, or drag and drop
2. Navigate to: `C:\Users\pwver\Documents\Hobby\Battery Tester\Batt Tester\dist`
3. Select `BatteryTester.exe`
4. Wait for upload to complete (it's 63MB, may take a minute)

Optional: Rename before uploading
- Copy `dist\BatteryTester.exe` to `dist\BatteryTester-Windows-v2.0.1.exe`
- Upload the renamed file

### 5. Publish

- Make sure "Set as the latest release" is checked
- Click **"Publish release"** (green button at bottom)

Done! Your release is now live at:
https://github.com/paulvee/Battery-tester-Python/releases/tag/v2.0.1

---

## Option B: Using GitHub CLI (Advanced)

### Prerequisites

Install GitHub CLI: https://cli.github.com/

### Authenticate

```powershell
gh auth login
```

Follow prompts to login with your GitHub account.

### Create Release with CLI

```powershell
cd "C:\Users\pwver\Documents\Hobby\Battery Tester\Batt Tester"

# Copy and rename executable
Copy-Item "dist\BatteryTester.exe" "dist\BatteryTester-Windows-v2.0.1.exe"

# Create release and upload
gh release create v2.0.1 `
  "dist\BatteryTester-Windows-v2.0.1.exe" `
  --title "Battery Tester v2.0.1 - Windows Release" `
  --notes-file RELEASE_NOTES.md
```

---

## Verification

After creating the release:

1. Visit: https://github.com/paulvee/Battery-tester-Python/releases
2. You should see your release listed
3. Click on it to view details
4. The executable should be listed under "Assets"
5. Anyone can now download it by clicking the file

## Notify Users

Let Bud know:
```
Release is live! Download link:
https://github.com/paulvee/Battery-tester-Python/releases/download/v2.0.1/BatteryTester-Windows-v2.0.1.exe
```

---

## Future Releases

For subsequent releases (v2.0.2, v2.1.0, etc.):

1. Update version in your code if applicable
2. Rebuild executable: `python build_executable.py`
3. Create new release with new tag (v2.0.2)
4. Upload new binary
5. Update release notes

See `release_helper.py` for an automated helper script.
