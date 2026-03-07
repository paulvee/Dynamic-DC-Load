#!/usr/bin/env python3
"""
Convert README.md to PDF
"""

import markdown
from xhtml2pdf import pisa
from pathlib import Path

def convert_readme_to_pdf():
    """Convert README.md to PDF with styling"""
    
    # Read markdown file
    readme_path = Path(__file__).parent / "README.md"
    with open(readme_path, 'r', encoding='utf-8') as f:
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
        <title>Battery Tester - Python Version</title>
        <style>
            @page {{
                size: A4;
                margin: 2cm;
            }}
            body {{
                font-family: Arial, Helvetica, sans-serif;
                line-height: 1.6;
                color: #333;
                font-size: 11pt;
            }}
            h1 {{
                color: #2c3e50;
                border-bottom: 3px solid #3498db;
                padding-bottom: 10px;
                margin-top: 20px;
                font-size: 24pt;
            }}
            h2 {{
                color: #34495e;
                border-bottom: 2px solid #95a5a6;
                padding-bottom: 8px;
                margin-top: 30px;
                font-size: 18pt;
            }}
            h3 {{
                color: #555;
                margin-top: 20px;
                font-size: 14pt;
            }}
            code {{
                background-color: #f4f4f4;
                padding: 2px 6px;
                border-radius: 3px;
                font-family: Courier, monospace;
                font-size: 9pt;
            }}
            pre {{
                background-color: #f8f8f8;
                border: 1px solid #ddd;
                border-radius: 5px;
                padding: 15px;
                overflow-x: auto;
                font-family: Courier, monospace;
                font-size: 9pt;
            }}
            pre code {{
                background-color: transparent;
                padding: 0;
            }}
            ul {{
                padding-left: 25px;
            }}
            li {{
                margin: 8px 0;
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
            }}
            table {{
                border-collapse: collapse;
                width: 100%;
                margin: 15px 0;
            }}
            th, td {{
                border: 1px solid #ddd;
                padding: 10px;
                text-align: left;
            }}
            th {{
                background-color: #3498db;
                color: white;
            }}
            .footer {{
                margin-top: 40px;
                padding-top: 20px;
                border-top: 1px solid #ddd;
                font-size: 9pt;
                color: #888;
                text-align: center;
            }}
        </style>
    </head>
    <body>
        {html_content}
        <div class="footer">
            Generated on March 6, 2026 | Battery Tester Python Documentation
        </div>
    </body>
    </html>
    """
    
    # Generate PDF
    output_path = Path(__file__).parent / "Battery_Tester_README.pdf"
    
    with open(output_path, 'wb') as pdf_file:
        # Convert HTML to PDF
        pisa_status = pisa.CreatePDF(
            full_html.encode('utf-8'),
            dest=pdf_file,
            encoding='utf-8'
        )
    
    if pisa_status.err:
        print(f"❌ Error creating PDF: {pisa_status.err}")
        return None
    
    print(f"✅ PDF created successfully: {output_path}")
    return output_path

if __name__ == "__main__":
    convert_readme_to_pdf()
