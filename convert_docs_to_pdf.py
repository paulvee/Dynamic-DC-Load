#!/usr/bin/env python3
"""
Convert CHANGELOG.md and TEST_PLAN.md to PDF
"""

import markdown
from xhtml2pdf import pisa
from pathlib import Path
from datetime import datetime

def convert_markdown_to_pdf(md_file_path, pdf_file_path, title):
    """Convert a markdown file to PDF with styling"""
    
    # Read markdown file
    with open(md_file_path, 'r', encoding='utf-8') as f:
        md_content = f.read()
    
    # Convert markdown to HTML
    html_content = markdown.markdown(
        md_content,
        extensions=['extra', 'codehilite', 'tables', 'fenced_code']
    )
    
    # Add HTML structure and CSS styling
    full_html = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <meta charset="utf-8">
        <title>{title}</title>
        <style>
            @page {{
                size: A4;
                margin: 2cm;
            }}
            body {{
                font-family: Arial, Helvetica, sans-serif;
                line-height: 1.6;
                color: #333;
                font-size: 10pt;
            }}
            h1 {{
                color: #2c3e50;
                border-bottom: 3px solid #3498db;
                padding-bottom: 10px;
                margin-top: 20px;
                font-size: 22pt;
                page-break-after: avoid;
            }}
            h2 {{
                color: #34495e;
                border-bottom: 2px solid #95a5a6;
                padding-bottom: 8px;
                margin-top: 25px;
                font-size: 16pt;
                page-break-after: avoid;
            }}
            h3 {{
                color: #555;
                margin-top: 18px;
                font-size: 13pt;
                page-break-after: avoid;
            }}
            h4 {{
                color: #666;
                margin-top: 15px;
                font-size: 11pt;
                page-break-after: avoid;
            }}
            code {{
                background-color: #f4f4f4;
                padding: 2px 5px;
                border-radius: 3px;
                font-family: Courier, monospace;
                font-size: 9pt;
            }}
            pre {{
                background-color: #f8f8f8;
                border: 1px solid #ddd;
                border-radius: 5px;
                padding: 12px;
                overflow-x: auto;
                font-family: Courier, monospace;
                font-size: 8pt;
                page-break-inside: avoid;
            }}
            pre code {{
                background-color: transparent;
                padding: 0;
            }}
            ul, ol {{
                padding-left: 25px;
            }}
            li {{
                margin: 6px 0;
            }}
            a {{
                color: #3498db;
                text-decoration: none;
            }}
            blockquote {{
                border-left: 4px solid #3498db;
                padding-left: 15px;
                margin-left: 0;
                color: #555;
                font-style: italic;
            }}
            table {{
                border-collapse: collapse;
                width: 100%;
                margin: 15px 0;
                font-size: 9pt;
                page-break-inside: avoid;
            }}
            th, td {{
                border: 1px solid #ddd;
                padding: 8px;
                text-align: left;
            }}
            th {{
                background-color: #3498db;
                color: white;
                font-weight: bold;
            }}
            tr:nth-child(even) {{
                background-color: #f9f9f9;
            }}
            strong {{
                color: #2c3e50;
            }}
            .footer {{
                margin-top: 40px;
                padding-top: 20px;
                border-top: 1px solid #ddd;
                font-size: 9pt;
                color: #888;
                text-align: center;
            }}
            hr {{
                border: none;
                border-top: 1px solid #ddd;
                margin: 20px 0;
            }}
            /* Prevent orphans and widows */
            p {{
                orphans: 3;
                widows: 3;
            }}
        </style>
    </head>
    <body>
        {html_content}
        <div class="footer">
            Generated on {datetime.now().strftime("%B %d, %Y")} | Dynamic DC Load Firmware Documentation
        </div>
    </body>
    </html>
    """
    
    # Generate PDF
    with open(pdf_file_path, 'wb') as pdf_file:
        # Convert HTML to PDF
        pisa_status = pisa.CreatePDF(
            full_html.encode('utf-8'),
            dest=pdf_file,
            encoding='utf-8'
        )
    
    if pisa_status.err:
        print(f"❌ Error creating PDF for {md_file_path.name}: {pisa_status.err}")
        return False
    
    print(f"✅ PDF created successfully: {pdf_file_path}")
    return True

def main():
    """Convert both CHANGELOG.md and TEST_PLAN.md to PDF"""
    
    base_path = Path(__file__).parent
    
    # List of files to convert
    files_to_convert = [
        {
            'input': base_path / "CHANGELOG.md",
            'output': base_path / "CHANGELOG.pdf",
            'title': "Dynamic DC Load Firmware - Changelog"
        },
        {
            'input': base_path / "TEST_PLAN.md",
            'output': base_path / "TEST_PLAN.pdf",
            'title': "Dynamic DC Load Firmware - Test Plan"
        }
    ]
    
    print("Converting markdown files to PDF...\n")
    
    success_count = 0
    for file_info in files_to_convert:
        if file_info['input'].exists():
            if convert_markdown_to_pdf(
                file_info['input'],
                file_info['output'],
                file_info['title']
            ):
                success_count += 1
        else:
            print(f"❌ File not found: {file_info['input']}")
    
    print(f"\n✅ Successfully converted {success_count} of {len(files_to_convert)} files")
    
    if success_count == len(files_to_convert):
        print("\n📧 PDFs are ready to send to Bud!")

if __name__ == "__main__":
    main()
