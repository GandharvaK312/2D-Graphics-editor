import subprocess
import os

def run_test():
    print("Starting integration test for editor.exe...")
    
    # Clean up any leftover file
    if os.path.exists("test_drawing.txt"):
        os.remove("test_drawing.txt")
        
    # Start editor.exe
    proc = subprocess.Popen(
        ["./editor.exe"],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )
    
    # Input sequences:
    # 4 -> list details
    # \n -> return
    # 1 -> add shape
    # 1 -> line
    # 10 -> X1
    # 5 -> Y1
    # 40 -> X2
    # 18 -> Y2
    # 5 -> save
    # test_drawing.txt -> filename
    # \n -> return
    # 7 -> clear canvas
    # 1 -> confirm
    # \n -> return
    # 6 -> load
    # test_drawing.txt -> filename
    # \n -> return
    # 8 -> exit
    inputs = [
        "4",   # List shapes
        "",    # Press enter
        "1",   # Add shape
        "1",   # Line
        "10",  # X1
        "5",   # Y1
        "40",  # X2
        "18",  # Y2
        "5",   # Save file
        "test_drawing.txt",
        "",    # Press enter
        "7",   # Clear canvas
        "1",   # Confirm clear
        "",    # Press enter
        "6",   # Load canvas
        "test_drawing.txt",
        "",    # Press enter
        "8"    # Exit
    ]
    
    input_str = "\n".join(inputs) + "\n"
    
    try:
        stdout, stderr = proc.communicate(input=input_str, timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
        stdout, stderr = proc.communicate()
        print("Test timed out!")
        print("STDOUT:")
        print(stdout)
        print("STDERR:")
        print(stderr)
        return False
        
    print("Process exited with code:", proc.returncode)
    
    # Verify file test_drawing.txt exists and contains expected content
    if not os.path.exists("test_drawing.txt"):
        print("FAIL: test_drawing.txt was not created!")
        return False
        
    with open("test_drawing.txt", "r") as f:
        content = f.read()
    print("Saved file contents:")
    print(content)
    
    lines = content.splitlines()
    if len(lines) < 2:
        print("FAIL: drawing file is empty or too short")
        return False
        
    # We expect next ID to be 4 (default objects 1 and 2, line added is 3)
    # Total active objects: 3
    # Record for line: '3 1 10 5 40 18'
    if lines[0].strip() != "4":
        print(f"FAIL: next ID mismatch, expected '4', got '{lines[0]}'")
        return False
    if lines[1].strip() != "3":
        print(f"FAIL: total count mismatch, expected '3', got '{lines[1]}'")
        return False
    
    line_record_found = False
    for line in lines[2:]:
        if line.strip() == "3 1 10 5 40 18":
            line_record_found = True
            break
            
    if not line_record_found:
        print("FAIL: line shape record missing or incorrect")
        return False
        
    print("PASS: integration test successful!")
    return True

if __name__ == "__main__":
    success = run_test()
    if not success:
        exit(1)
