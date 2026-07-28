import os
import subprocess
import sys
from postprocess_word_fields import process_document

SAMPUL_MD = 'CNCTA/TA-Rombak/00-sampul.md'
OUTPUT_RAW = 'CNCTA/output/sampul-raw.docx'
OUTPUT_FINAL = 'CNCTA/output/00-sampul.docx'
RESOURCE_PATH = os.path.abspath('CNCTA/TA-Rombak')
REF_COVER = 'CNCTA/reference_cover.docx'

def main():
    os.makedirs('CNCTA/output', exist_ok=True)
    
    print(f"Building {SAMPUL_MD} -> {OUTPUT_FINAL}...")
    cmd = [
        'pandoc',
        '--defaults', 'CNCTA/pandoc-defaults.yaml',
        '--reference-doc', REF_COVER if os.path.exists(REF_COVER) else 'CNCTA/reference1.docx',
        '--resource-path', RESOURCE_PATH,
        '-o', OUTPUT_RAW,
        SAMPUL_MD
    ]
    
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Error building sampul:\n{res.stderr}")
        return
        
    print("Postprocessing sampul document...")
    try:
        process_document(OUTPUT_RAW, OUTPUT_FINAL)
        out_path = OUTPUT_FINAL
    except PermissionError:
        out_path = 'CNCTA/output/00-sampul-fixed.docx'
        process_document(OUTPUT_RAW, out_path)
        
    if os.path.exists(OUTPUT_RAW):
        os.remove(OUTPUT_RAW)
        
    print(f"SUCCESS! Sampul built at: {out_path}")

if __name__ == '__main__':
    main()
