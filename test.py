import csv
with open('Test_Cases/eazy_test_cases/shanghai_test_cases/case2_medium/map_0800.csv', 'r', encoding='utf-8') as f:    
    reader = csv.reader(f)
    header = next(reader)
    for i, row in enumerate(reader):
        if i < 3:
            print(f'Row {i}: start=[{row[1]}], end=[{row[2]}]')
            print(f'  start repr: {repr(row[1])}, end repr: {repr(row[2])}')