import glob
import re
import os

def clean_image_attributes(content):
    def replace_height(match):
        img_prefix = match.group(1) # ![alt](path)
        attrs = match.group(2)      # attr inside {...}
        
        # Remove height="..." or height='...'
        new_attrs = re.sub(r'\s*height=[\"\'][^\"\']*[\"\']', '', attrs)
        # Clean up whitespace
        new_attrs = re.sub(r'\s+', ' ', new_attrs).strip()
        
        return f"{img_prefix}{{{new_attrs}}}"

    pattern = r'(!\[[^\]]*\]\([^\)]+\))\{([^\}]+)\}'
    return re.sub(pattern, replace_height, content)

def main():
    md_files = sorted(glob.glob(r'd:\Github\CNCProject\CNCTA\TA-Rombak\*.md'))
    modified_count = 0
    
    for filepath in md_files:
        with open(filepath, 'r', encoding='utf-8') as f:
            original = f.read()
        
        updated = clean_image_attributes(original)
        if original != updated:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(updated)
            print(f"Updated: {os.path.basename(filepath)}")
            modified_count += 1
            
    print(f"Complete! Removed height attributes from {modified_count} markdown files.")

if __name__ == "__main__":
    main()
