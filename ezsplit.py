import os
import sys

def exec_split(dir_name):

  # list files with size
  files = []
  for file_name in os.listdir(dir_name):
    if (file_name[0] == '.'):
      continue
    st = os.stat(dir_name + "\\" + file_name)
    mode = st[0]
    size = st[6]
    print(st.is_dir())
    files.append((file_name, size, st[0], st[1], st[2], st[3], st[4], st[5]))

  # sort by size
  sorted_files = sorted(files, key=lambda x: x[1], reverse=True)

  for f in sorted_files:
    print(f)

def main():
  if len(sys.argv) < 2:
    print("usage: micropytyon ezsplit.py <directory>")
    sys.exit(1)

  exec_split(sys.argv[1])

if __name__ == "__main__":
  main()