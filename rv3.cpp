
#include <iostream>

const char regnames[32][5] = {
    "zero", "ra",   "sp",   "gp",   "tp",   "t0",   "t1",   "t2",
    "s0",   "s1",   "a0",   "a1",   "a2",   "a3",   "a4",   "a5",
    "a6",   "a7",   "s2",   "s3",   "s4",   "s5",   "s6",   "s7",
    "s8",   "s9",   "s10",  "s11",  "t3",   "t4",   "t5",   "t6"};

enum  cmd_fmt {R, IL, I3, I0, S, B, U, J};

const char* getbindstr(unsigned char inf) {
    switch (inf >> 4) {
    case 0: return "LOCAL";
    case 1: return "GLOBAL";
    case 2: return "WEAK";
    case 13: return "LOPROC";
    case 15: return "HIPROC";
    default: return "UNDEF";
    }
}

const char* gettypestr(unsigned char inf) {
    switch (inf & 0b1111) {
    case 0: return "NOTYPE";
    case 1: return "OBJECT";
    case 2: return "FUNC";
    case 3: return "SECTION";
    case 4: return "FILE";
    case 13: return "LOPROC";
    case 15: return "HIPROC";
    default: return "UNDEF";
    }
}

void getindexstr(unsigned short idx, char* rstr) {
    switch (idx) {
    case 0: {strcpy_s(rstr,10,"UNDEF"); break; }
    case 0xFF00: {strcpy_s(rstr, 10, "LOPROC"); break;}
    case 0xFF1F: {strcpy_s(rstr, 10, "HIPROC"); break;}
    case 0xFFF1: {strcpy_s(rstr, 10, "ABS"); break;}
    case 0xFFF2: {strcpy_s(rstr, 10, "COMMON"); break; }
    case 0xFFFF: {strcpy_s(rstr, 10, "HIRESERVE"); break; }
    default: {
        sprintf_s(rstr, 10, "%d", idx);
    }
    }
}

unsigned int op_opcode(unsigned int cmd) {
    return (cmd & 0b1111111);
}

unsigned int op_rd(unsigned int cmd) {
    return ((cmd >> 7) & 0b11111);
}

unsigned int op_rs1(unsigned int cmd) {
    return ((cmd >> 15) & 0b11111);
}

unsigned int op_rs2(unsigned int cmd) {
    return ((cmd >> 20) & 0b11111);
}

unsigned int op_funct3(unsigned int cmd) {
    return ((cmd >> 12) & 0b111);
}

unsigned int op_funct7(unsigned int cmd) {
    return (cmd >> 25);
}

int op_Iimm(unsigned int cmd) {
    return (((long long)cmd<<32) >> 52);
}

int op_Simm(unsigned int cmd) {
    return (((cmd >> 7) & 0b11111)|((((long long)cmd<<32) >> 57)<<5));
}

int op_Bimm(unsigned int cmd) {
    return (((cmd >> 7) & 0b11110) | ((cmd >> 20) & 0b11111100000) | ((cmd << 4)&(1<<11)) | ((((long long)cmd<<32)>>63)<<12));
}

int op_Uimm(unsigned int cmd) {
    return ((((long long)cmd<<32)>>44)<<12);
}

int op_Jimm(unsigned int cmd) {
    return ((cmd & 0xFF000) | ((cmd >> 20) & 0x7FE) | ((cmd >>9) & (1 << 11)) | ((((long long)cmd<<32) >> 63)<<20));
}

const char *cmdname(unsigned int cmd) {
    switch (op_opcode(cmd)) {
    case 0x03: {
        switch (op_funct3(cmd)) {
        case 0b000: return "lb";
        case 0b001: return "lh";
        case 0b010: return "lw";
        case 0b100: return "lbu";
        case 0b101: return "lhu";
        }
        return "unknown_instruction";
    }
    case 0x0f: return "fence";
    case 0x13: {
        switch (op_funct3(cmd)) {
        case 0b000: return "addi";
        case 0b001: return "slli";
        case 0b010: return "slti";
        case 0b011: return "sltiu";
        case 0b100: return "xori";
        case 0b101: return op_funct7(cmd) ? "srai" : "srli";
        case 0b110: return "ori";
        case 0b111: return "andi";
        }
        return "unknown_instruction";
    }
    case 0x17: return "auipc";
    case 0x23: {
        switch (op_funct3(cmd)) {
        case 0b000: return "sb";
        case 0b001: return "sh";
        case 0b010: return "sw";
        }
        return "unknown_instruction";
    }
    case 0x33: {
        if (op_funct7(cmd) == 1)
            switch (op_funct3(cmd)) {
            case 0b000: return "mul";
            case 0b001: return "mulh";
            case 0b010: return "mulhsu";
            case 0b011: return "mulhu";
            case 0b100: return "div";
            case 0b101: return "divu";
            case 0b110: return "rem";
            case 0b111: return "remu";
            }
        else switch (op_funct3(cmd)) {
        case 0b000: return op_funct7(cmd) ? "sub" : "add";
        case 0b001: return "sll";
        case 0b010: return "slt";
        case 0b011: return "sltu";
        case 0b100: return "xor";
        case 0b101: return op_funct7(cmd) ? "sra" : "srl";
        case 0b110: return "or";
        case 0b111: return "and";
        }
        return "unknown_instruction";
    }
    case 0x37: return "lui";
    case 0x63: {
        switch (op_funct3(cmd)) {
        case 0b000: return "beq";
        case 0b001: return "bne";
        case 0b100: return "blt";
        case 0b101: return "bge";
        case 0b110: return "bltu";
        case 0b111: return "bgeu";
        }
        return "unknown_instruction";
    }
    case 0x67: return "jalr";
    case 0x6F: return "jal";
    case 0x73: return op_funct7(cmd) ? "ebreak" : "ecall";
    }
    return "unknown_instruction";
}

cmd_fmt cmdtype(unsigned int cmd) {
    switch (op_opcode(cmd)) {
    case 0x03: return IL;
    case 0x0f: return I0;
    case 0x13: return I3;
    case 0x17: return U;
    case 0x23: return S;
    case 0x33: return R;
    case 0x37: return U;
    case 0x63: return B;
    case 0x67: return I3;
    case 0x6F: return J;
    case 0x73: return I0;
    }
    return I0;
}


typedef struct elf32_header{
    unsigned char	e_magic[4];
    unsigned char	e_class;
    unsigned char	e_data;
    unsigned char	e_version;
    unsigned char	e_osabi;
    unsigned char	e_abiversion;
    unsigned char	e_pad[7];
    unsigned short	e_type;
    unsigned short	e_machine;
    unsigned int	e_version2;
    unsigned int	e_entry;
    unsigned int	e_phoff;
    unsigned int	e_shoff;
    unsigned int	e_flags;
    unsigned short	e_ehsize;
    unsigned short	e_phentsize;
    unsigned short	e_phnum;
    unsigned short	e_shentsize;
    unsigned short	e_shnum;
    unsigned short	e_shstrndx;
} Elf32_Header;

typedef struct elf32_pheader {
    unsigned int	p_type;
    unsigned int	p_offset;
    unsigned int	p_vaddr;
    unsigned int	p_paddr;
    unsigned int	p_filesz;
    unsigned int	p_memsz;
    unsigned int	p_flags;
    unsigned int	p_align;
} Elf32_ProgHeader;

typedef struct elf32_sheader {
    unsigned int	sh_name;
    unsigned int	sh_type;
    unsigned int	sh_flags;
    unsigned int	sh_addr;
    unsigned int	sh_offset;
    unsigned int	sh_size;
    unsigned int	sh_link;
    unsigned int	sh_info;
    unsigned int	sh_addralign;
    unsigned int	sh_entsize;
} Elf32_SecHeader;

typedef struct elf32_symbol {
    unsigned int	st_name;
    unsigned int	st_value;
    unsigned int	st_size;
    unsigned char	st_info;
    unsigned char	st_other;
    unsigned short	st_shndx;
} Elf32_Symbol;


int  i,flab,msymtab,symcount,mtext;
unsigned int tc, txtoff, txtlen, tcmd, pc;

void qs(unsigned int* arr, int first, int last)
{
    if (first < last)
    {
        int left = first, right = last, middle = arr[(left + right) / 2];
        do
        {
            while (arr[left] < middle) left++;
            while (arr[right] > middle) right--;
            if (left <= right)
            {
                unsigned int tmp = arr[left];
                arr[left] = arr[right];
                arr[right] = tmp;
                left++;
                right--;
            }
        } while (left <= right);
        qs(arr, first, right);
        qs(arr, left, last);
    }
}

int main(int argc, char* argv[])
{
    FILE* ef,*af;
    int i;
    size_t readed;
    Elf32_Header ElfHeader;

    if (argc < 2) {
        printf("Too few paramaters.\n");
        exit(-1);
    }
    if ((fopen_s(&ef, argv[1], "rb")) != 0) {
        printf("Unable to open input file\n");
        exit(-1);
    }

    if ((fopen_s(&af, argv[2], "wt")) != 0) {
        printf("Unable to open output file\n");
        exit(-1);
    }

    readed = fread(&ElfHeader, 1, sizeof(Elf32_Header), ef);
    if (readed < sizeof(Elf32_Header)) {
        printf("Unexpected length of file\n");
        exit(-1);
    }
    if ((ElfHeader.e_magic[0] != 0x7F) || (ElfHeader.e_magic[1] != 0x45) || (ElfHeader.e_magic[2] != 0x4C) || (ElfHeader.e_magic[3] != 0x46) || (ElfHeader.e_class != 1)) {
        printf("Unsupported file type\n");
        exit(-1);
    }
    if (ElfHeader.e_machine != 0xF3) {
        printf("Unsupported processor type\n");
        exit(-1);
    }

    Elf32_ProgHeader* ProgHeads = new Elf32_ProgHeader[ElfHeader.e_phnum];
    fseek(ef, ElfHeader.e_phoff, SEEK_SET);
    fread(ProgHeads, ElfHeader.e_phentsize, ElfHeader.e_phnum, ef);

    Elf32_SecHeader* SecHeads = new Elf32_SecHeader[ElfHeader.e_shnum];
    fseek(ef, ElfHeader.e_shoff, SEEK_SET);
    fread(SecHeads, ElfHeader.e_shentsize, ElfHeader.e_shnum, ef);

    char* SecNames = new char[SecHeads[ElfHeader.e_shstrndx].sh_size];
    fseek(ef, SecHeads[ElfHeader.e_shstrndx].sh_offset, SEEK_SET);
    fread(SecNames, SecHeads[ElfHeader.e_shstrndx].sh_size, 1, ef);

    for (i = 0; i < ElfHeader.e_shnum; i++) {
        if (SecHeads[i].sh_type == 2) msymtab = i;
        if (strcmp(&SecNames[SecHeads[i].sh_name], ".text") == 0) mtext = i;
    }

    char* SymNames = new char[SecHeads[SecHeads[msymtab].sh_link].sh_size];
    fseek(ef, SecHeads[SecHeads[msymtab].sh_link].sh_offset, SEEK_SET);
    fread(SymNames, SecHeads[SecHeads[msymtab].sh_link].sh_size, 1, ef);

    symcount = SecHeads[msymtab].sh_size / SecHeads[msymtab].sh_entsize;
    Elf32_Symbol* Symbols = new Elf32_Symbol[symcount];
    fseek(ef, SecHeads[msymtab].sh_offset, SEEK_SET);
    fread(Symbols, 1, SecHeads[msymtab].sh_size, ef);

    txtlen = SecHeads[mtext].sh_size >> 2;
    txtoff = SecHeads[mtext].sh_addr;
    unsigned int* txt = new unsigned int[txtlen];
    unsigned int* mlabels = new unsigned int[txtlen];
    fseek(ef, SecHeads[mtext].sh_offset, SEEK_SET);
    fread(txt, 4, txtlen, ef);

    int mlabcnt = 0;
    int fmlab = -1;
    int tc = 0;
    while (tc < txtlen) {
        pc = txtoff + tc * 4;
        switch (cmdtype(txt[tc])) {
        case B: {
            flab = -1;
            for (i = 0; i < symcount; i++) {
                if (((Symbols[i].st_info & 0x0f) != 3) && (Symbols[i].st_shndx == mtext) && ((op_Bimm(txt[tc]) + pc) == Symbols[i].st_value)) {
                    flab = i;
                    break;
                }
            }
            if (flab >= 0) break;
            mlabels[mlabcnt] = op_Bimm(txt[tc]) + pc;
            mlabcnt++;
            break;
        }
        case J: {
            flab = -1;
            for (i = 0; i < symcount; i++) {
                if (((Symbols[i].st_info & 0x0f) != 3) && (Symbols[i].st_shndx == mtext) && ((op_Jimm(txt[tc]) + pc) == Symbols[i].st_value)) {
                    flab = i;
                    break;
                }
            }
            if (flab >= 0) break;
            mlabels[mlabcnt] = op_Jimm(txt[tc]) + pc;
            mlabcnt++;
            break;
        }
        };
        tc++;
    }

    if (mlabcnt>1) qs(mlabels, 0, mlabcnt - 1);

    fprintf(af,".text\n");
    tc = 0;
    while (tc < txtlen) {
        pc = txtoff + tc * 4;
        for (i = 0; i < symcount; i++) {
            if (((Symbols[i].st_info & 0x0f) != 3) && (Symbols[i].st_shndx == mtext) && (pc == Symbols[i].st_value)) {
                fprintf(af, "%08x   <%s>:\n", pc, &SymNames[Symbols[i].st_name]);
            }
        }
        for (i = 0; i < mlabcnt; i++) {
            if (pc == mlabels[i]) {
                fprintf(af, "%08x   <L%d>:\n", pc, i);
            }
        }
        switch (cmdtype(txt[tc])) {
        case R: fprintf(af, "   %05x:\t%08x\t%7s\t%s, %s, %s\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rd(txt[tc])], regnames[op_rs1(txt[tc])], regnames[op_rs2(txt[tc])]); break;
        case IL: fprintf(af, "   %05x:\t%08x\t%7s\t%s, %d(%s)\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rd(txt[tc])], op_Iimm(txt[tc]), regnames[op_rs1(txt[tc])]); break;
        case I3: fprintf(af, "   %05x:\t%08x\t%7s\t%s, %s, %d\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rd(txt[tc])], regnames[op_rs1(txt[tc])], op_Iimm(txt[tc])); break;
        case I0: fprintf(af, "   %05x:\t%08x\t%7s\n", pc, txt[tc], cmdname(txt[tc])); break;
        case S: fprintf(af, "   %05x:\t%08x\t%7s\t%s, %d(%s)\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rs2(txt[tc])], op_Simm(txt[tc]), regnames[op_rs1(txt[tc])]); break;
        case U: fprintf(af, "   %05x:\t%08x\t%7s\t%s, 0x%x\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rd(txt[tc])], op_Uimm(txt[tc])>>12); break;
        case J: {
            flab = -1;
            for (i = 0; i < symcount; i++) {
                if (((Symbols[i].st_info & 0x0f) != 3) && (Symbols[i].st_shndx == mtext) && ((op_Jimm(txt[tc])+pc) == Symbols[i].st_value)) {
                    flab = i;
                    break;
                }
            }
            fmlab = -1;
            for (i = 0; i < mlabcnt; i++) {
                if ((op_Jimm(txt[tc]) + pc) == mlabels[i]) {
                    fmlab = i;
                    break;
                }
            }
            if (flab >= 0) fprintf(af, "   %05x:\t%08x\t%7s\t%s, %08x <%s>\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rd(txt[tc])], op_Jimm(txt[tc]) + pc, &SymNames[Symbols[flab].st_name]);
            else if (fmlab >= 0) fprintf(af, "   %05x:\t%08x\t%7s\t%s, %08x <L%d>\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rd(txt[tc])], op_Jimm(txt[tc]) + pc, fmlab);
            else fprintf(af, "   %05x:\t%08x\t%7s\t%s, %08x\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rd(txt[tc])], op_Jimm(txt[tc])+pc);
            break;
        }
        case B: {
            flab = -1;
            for (i = 0; i < symcount; i++) {
                if (((Symbols[i].st_info & 0x0f) != 3) && (Symbols[i].st_shndx == mtext) && ((op_Bimm(txt[tc]) + pc) == Symbols[i].st_value)) {
                    flab = i;
                    break;
                }
            }
            fmlab = -1;
            for (i = 0; i < mlabcnt; i++) {
                if ((op_Bimm(txt[tc]) + pc) == mlabels[i]) {
                    fmlab = i;
                    break;
                }
            }
            if (flab >= 0) fprintf(af, "   %05x:\t%08x\t%7s\t%s, %s, %08x <%s>\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rs1(txt[tc])], regnames[op_rs2(txt[tc])], op_Bimm(txt[tc]) + pc, &SymNames[Symbols[flab].st_name]);
            else if (fmlab >= 0) fprintf(af, "   %05x:\t%08x\t%7s\t%s, %s, %08x <L%d>\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rs1(txt[tc])], regnames[op_rs2(txt[tc])], op_Bimm(txt[tc]) + pc, fmlab);
            else fprintf(af, "   %05x:\t%08x\t%7s\t%s, %s, %08x\n", pc, txt[tc], cmdname(txt[tc]), regnames[op_rs1(txt[tc])], regnames[op_rs2(txt[tc])], op_Bimm(txt[tc]) + pc);
            break;
        }
        default:fprintf(af, "\t%08x: %s\n", pc, cmdname(txt[tc]));
        };
        
        tc++;
    }
    fprintf(af, "\n.symtab\nSymbol Value          	Size Type 	Bind 	Vis   	Index Name\n");
    char indexstr[10];
    for (i = 0; i < symcount; i++) {
        getindexstr(Symbols[i].st_shndx,indexstr);
        fprintf(af, "[%4i] 0x%-15X %5i %-8s %-8s %-8s %6s %s\n", i, Symbols[i].st_value, Symbols[i].st_size, gettypestr(Symbols[i].st_info), getbindstr(Symbols[i].st_info),"DEFAULT", indexstr, &SymNames[Symbols[i].st_name]);
    }
    fclose(ef);
    fclose(af);
}

