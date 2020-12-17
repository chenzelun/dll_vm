buf = ['']
while True:
    line = input()
    if line == 'zxc':
        break
    buf.append(line)

with open('./dex/register_interpret_builder.py', 'r') as reader:
    dst = reader.readlines()

dst_idx = 102
src_idx = 0

while src_idx < len(buf):
    while src_idx < len(buf) and not buf[src_idx].strip().startswith('void ST_CH'):
        src_idx += 1

    if src_idx >= len(buf):
        break

    src_idx += 1
    while src_idx < len(buf) and buf[src_idx].strip().startswith('vmc->tmp'):
        src_idx += 1

    if buf[src_idx].strip() == '':
        src_idx += 1

    src_start = src_idx
    while src_idx < len(buf) and not buf[src_idx].strip().startswith('void ST_CH'):
        src_idx += 1

    src_idx -= 1
    while buf[src_idx].strip() != '}':
        src_idx -= 1

    src_end = src_idx
    if buf[src_end - 1].strip().startswith('vmc->pc_off'):
        src_end -= 1

    data = []
    while src_start < src_end:
        line = buf[src_start].strip()
        while line.endswith(','):
            src_start += 1
            line += ' ' + buf[src_start].strip()
        data.append('r"""' + line + '""",\n')
        src_start += 1

    while not dst[dst_idx].strip().startswith('self.handle'):
        dst_idx += 1
    dst_idx += 1
    dst_start = dst_idx
    while dst[dst_idx].strip() != ']':
        dst_idx += 1
    dst_end = dst_idx

    dst[dst_start:dst_end] = data
    dst_idx = dst_start + len(data) + 5

with open('./dex/register_interpret_builder.py', 'w') as writer:
    writer.writelines(dst)
