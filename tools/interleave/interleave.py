#!/usr/bin/env python3
"""
interleave - explore EVERY interleaving of a small concurrent program.

A stress test runs a program a few thousand times and hopes to hit the bad
schedule. This tool instead tries all of them, so its answers are exact:

  * mutual exclusion  - can two threads be in their critical sections at once?
  * deadlock          - can every thread end up waiting?
  * blocked forever   - can a thread wait forever while another one stays in
                        its non-critical section? (e.g. strict alternation)
  * livelock          - can threads keep running forever with nobody entering
                        the critical section (under a fair scheduler)?
  * starvation        - can one thread be overtaken forever while others enter?
  * outputs           - every possible output of a program with `print`
                        ("Is output B B A possible?")
  * infinite outputs  - can the program print B A B A ... forever?

Programs are written in a small pseudocode language close to the lecture
slides and exam papers. See tools/interleave/README.md for the language.

Usage:
  interleave.py FILE.il                      protocol checks (default)
  interleave.py FILE.il --outputs            list all outputs of terminating runs
  interleave.py FILE.il --why "B B A"        show a run producing exactly that output
  interleave.py FILE.il --infinite "B A"     can the output be (B A)(B A)... forever?
  interleave.py FILE.il --step               choose the schedule yourself
Options:
  --no-halt          threads may NOT stop forever in `noncritical`
  --priority A=2,C=1 strict priority scheduler (single core): only the
                     highest-priority runnable threads may run
  --bound N          prune states where any integer exceeds N in size
  --max-states N     give up after N states (default 500000)
  --max-output N     longest output kept in --outputs mode (default 12)
  --show N           how many outputs to list in --outputs mode (default 30)
  --verdict          print a one-line machine-readable VERDICT (for tests)
"""

import argparse
import re
import sys
from collections import deque

# --------------------------------------------------------------------------
# Language front end
# --------------------------------------------------------------------------

SAFE_BUILTINS = {
    "max": max, "min": min, "abs": abs, "len": len, "any": any, "all": all,
    "range": range, "sum": sum, "True": True, "False": False,
}

SEM_DOWN = {"down", "wait", "P", "acquire", "lock"}
SEM_UP = {"up", "signal", "V", "release", "unlock"}


class ILError(Exception):
    pass


def translate_expr(text):
    """Accept C-ish operators (&&, ||, !, true/false) as well as Python's."""
    t = text.strip().rstrip(";").strip()
    t = t.replace("&&", " and ").replace("||", " or ")
    t = re.sub(r"!(?!=)", " not ", t)
    t = re.sub(r"\btrue\b", "True", t)
    t = re.sub(r"\bfalse\b", "False", t)
    return t


def code_names(code):
    names = set(code.co_names)
    for c in code.co_consts:
        if hasattr(c, "co_names"):
            names |= code_names(c)
    return names


class Expr:
    def __init__(self, text, line):
        self.text = text.strip()
        self.line = line
        py = translate_expr(text)
        try:
            self.code = compile(py, f"<line {line}>", "eval")
        except SyntaxError as e:
            raise ILError(f"line {line}: cannot understand expression '{text.strip()}' ({e.msg})")
        self.names = code_names(self.code)

    def __repr__(self):
        return self.text


class Instr:
    """One instruction. `kind` is one of:
    ASSIGN target idx expr | AWAIT expr | JIF expr target | JMP target |
    CRIT | NONCRIT | PRINT token | DOWN sem | UP sem | ASSERT expr |
    ATOMIC body | END"""

    def __init__(self, kind, line, src, **kw):
        self.kind = kind
        self.line = line
        self.src = src.strip()
        self.__dict__.update(kw)
        self.local_only = False


class Line:
    def __init__(self, no, indent, text):
        self.no, self.indent, self.text = no, indent, text


def strip_comment(s):
    out, q = [], None
    for ch in s:
        if q:
            out.append(ch)
            if ch == q:
                q = None
        elif ch in "\"'":
            q = ch
            out.append(ch)
        elif ch == "#":
            break
        else:
            out.append(ch)
    return "".join(out).rstrip()


def split_header(text):
    """'while x: y = 1' -> ('while x', 'y = 1'); returns (head, inline_or_None)."""
    depth, q = 0, None
    for k, ch in enumerate(text):
        if q:
            if ch == q:
                q = None
        elif ch in "\"'":
            q = ch
        elif ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth -= 1
        elif ch == ":" and depth == 0:
            rest = text[k + 1:].strip()
            return text[:k].strip(), (rest or None)
    raise ILError(f"expected ':' at the end of '{text}'")


class Program:
    def __init__(self, source, filename="<input>"):
        self.filename = filename
        self.consts = {}
        self.shared_names = []      # order of shared variables in the state
        self.shared_init = []
        self.sems = set()
        self.procs = {}             # name -> (params, [Line])
        self.thread_specs = []      # (name, param, value, [Line])
        self._parse(source)
        self.threads = []           # compiled: list of dicts
        for name, pname, pval, body in self.thread_specs:
            self.threads.append(self._compile_thread(name, pname, pval, body))

    # ---------------------------------------------------------------- parse
    def _parse(self, source):
        lines = []
        for no, raw in enumerate(source.splitlines(), 1):
            s = strip_comment(raw.replace("\t", "    "))
            if not s.strip():
                continue
            indent = len(s) - len(s.lstrip(" "))
            lines.append(Line(no, indent, s.strip()))
        k = 0
        while k < len(lines):
            ln = lines[k]
            if ln.indent != 0:
                raise ILError(f"line {ln.no}: unexpected indentation")
            body_end = k + 1
            while body_end < len(lines) and lines[body_end].indent > 0:
                body_end += 1
            body = lines[k + 1:body_end]
            self._top_level(ln, body)
            k = body_end

    def _eval_const(self, text, line):
        try:
            return eval(Expr(text, line).code, {"__builtins__": SAFE_BUILTINS, **self.consts})
        except ILError:
            raise
        except Exception as e:
            raise ILError(f"line {line}: cannot evaluate '{text}': {e}")

    def _top_level(self, ln, body):
        t = ln.text
        m = re.match(r"const\s+(\w+)\s*=\s*(.+)$", t)
        if m:
            self.consts[m.group(1)] = self._eval_const(m.group(2), ln.no)
            return
        m = re.match(r"(shared|int|bool|var)\s+(.+)$", t)
        if m:
            for decl in self._split_commas(m.group(2)):
                dm = re.match(r"(\w+)\s*(?:\[\s*(.+?)\s*\])?\s*(?:=\s*(.+))?$", decl.strip())
                if not dm:
                    raise ILError(f"line {ln.no}: bad declaration '{decl}'")
                name, size, init = dm.group(1), dm.group(2), dm.group(3) or "0"
                val = self._eval_const(init, ln.no)
                if size is not None:
                    n = self._eval_const(size, ln.no)
                    val = tuple(val for _ in range(n)) if not isinstance(val, (list, tuple)) else tuple(val)
                self._declare_shared(name, val, ln.no)
            return
        m = re.match(r"(sem|semaphore)\s+(.+)$", t)
        if m:
            for decl in self._split_commas(m.group(2)):
                dm = re.match(r"(\w+)\s*=\s*(.+)$", decl.strip())
                if not dm:
                    raise ILError(f"line {ln.no}: write 'sem name = initial_value'")
                self._declare_shared(dm.group(1), self._eval_const(dm.group(2), ln.no), ln.no)
                self.sems.add(dm.group(1))
            return
        m = re.match(r"mutex\s+(.+)$", t)
        if m:
            for name in self._split_commas(m.group(1)):
                self._declare_shared(name.strip(), 1, ln.no)
                self.sems.add(name.strip())
            return
        m = re.match(r"(?:proc|def)\s+(\w+)\s*\((.*?)\)\s*:$", t)
        if m:
            params = [p.strip() for p in m.group(2).split(",") if p.strip()]
            self.procs[m.group(1)] = (params, body)
            return
        m = re.match(r"thread\s+(\w+)\s*(?:\(\s*(\w+)\s*\)\s*for\s+\w+\s+in\s+(.+?)\.\.(.+?))?\s*:$", t)
        if m:
            name = m.group(1)
            if m.group(2):
                lo = self._eval_const(m.group(3), ln.no)
                hi = self._eval_const(m.group(4), ln.no)
                for v in range(lo, hi + 1):
                    self.thread_specs.append((f"{name}{v}", m.group(2), v, body))
            else:
                self.thread_specs.append((name, None, None, body))
            return
        raise ILError(f"line {ln.no}: expected const/shared/sem/mutex/proc/thread, got '{t}'")

    @staticmethod
    def _split_commas(s):
        parts, depth, cur = [], 0, ""
        for ch in s:
            if ch in "([{":
                depth += 1
            elif ch in ")]}":
                depth -= 1
            if ch == "," and depth == 0:
                parts.append(cur)
                cur = ""
            else:
                cur += ch
        if cur.strip():
            parts.append(cur)
        return parts

    def _declare_shared(self, name, val, line):
        if name in self.shared_names:
            raise ILError(f"line {line}: '{name}' declared twice")
        self.shared_names.append(name)
        self.shared_init.append(val)

    # -------------------------------------------------------------- compile
    def _compile_thread(self, name, pname, pval, body):
        th = {"name": name, "param": pname, "pval": pval, "locals": [], "local_init": [],
              "code": []}
        self._cur = th
        lines = list(body)
        # leading `local` declarations
        while lines and re.match(r"local\s", lines[0].text):
            ln = lines.pop(0)
            for decl in self._split_commas(ln.text[len("local"):]):
                dm = re.match(r"(\w+)\s*(?:=\s*(.+))?$", decl.strip())
                if not dm:
                    raise ILError(f"line {ln.no}: bad local declaration")
                env = {"__builtins__": SAFE_BUILTINS, **self.consts}
                if pname:
                    env[pname] = pval
                val = eval(Expr(dm.group(2) or "0", ln.no).code, env)
                self._add_local(dm.group(1), val)
        code = th["code"]
        self._block(lines, code, loops=[], proc_end=None)
        code.append(Instr("END", lines[-1].no if lines else 0, "(end of thread)"))
        self._mark_local_only(th)
        return th

    def _add_local(self, name, val=0):
        th = self._cur
        if name in th["locals"]:
            return
        if name in self.shared_names:
            raise ILError(f"local '{name}' in thread {th['name']} hides a shared variable")
        th["locals"].append(name)
        th["local_init"].append(val)

    def _check_names(self, expr):
        th = self._cur
        for n in expr.names:
            if n in SAFE_BUILTINS or n in self.consts or n in self.shared_names \
                    or n in th["locals"] or n == th["param"]:
                continue
            # generator variables (e.g. `k` in any(... for k in ...)) are fine
            if re.search(r"\bfor\s+" + re.escape(n) + r"\b", expr.text):
                continue
            raise ILError(f"line {expr.line}: unknown name '{n}' (declare it with shared/const/local)")

    def _block(self, lines, code, loops, proc_end):
        k = 0
        while k < len(lines):
            ln = lines[k]
            j = k + 1
            while j < len(lines) and lines[j].indent > ln.indent:
                j += 1
            child = lines[k + 1:j]
            k = j
            # if/elif/else chains are handled together
            if re.match(r"if\b", ln.text):
                branches = [(ln, child)]
                while k < len(lines) and lines[k].indent == ln.indent and \
                        re.match(r"(elif|else)\b", lines[k].text):
                    e = lines[k]
                    j = k + 1
                    while j < len(lines) and lines[j].indent > e.indent:
                        j += 1
                    branches.append((e, lines[k + 1:j]))
                    k = j
                self._if_chain(branches, code, loops, proc_end)
            else:
                self._stmt(ln, child, code, loops, proc_end)

    def _inline_body(self, ln, child, inline):
        if inline is not None:
            if child:
                raise ILError(f"line {ln.no}: use either an inline statement or an indented block")
            return [Line(ln.no, ln.indent + 4, inline)]
        if not child:
            raise ILError(f"line {ln.no}: missing indented block after ':'")
        return child

    def _if_chain(self, branches, code, loops, proc_end):
        end_jumps = []
        for idx, (ln, child) in enumerate(branches):
            head, inline = split_header(ln.text)
            body = self._inline_body(ln, child, inline)
            kw = head.split()[0]
            if kw == "else":
                self._block(body, code, loops, proc_end)
                continue
            cond = Expr(head[len(kw):], ln.no)
            self._check_names(cond)
            jif = Instr("JIF", ln.no, ln.text, expr=cond, target=None)
            code.append(jif)
            self._block(body, code, loops, proc_end)
            if idx < len(branches) - 1:
                jmp = Instr("JMP", ln.no, "(end of if)", target=None)
                code.append(jmp)
                end_jumps.append(jmp)
            jif.target = len(code)
        for jmp in end_jumps:
            jmp.target = len(code)

    def _stmt(self, ln, child, code, loops, proc_end):
        t = ln.text.rstrip(";").strip()
        th = self._cur

        def add(kind, **kw):
            ins = Instr(kind, ln.no, t, **kw)
            code.append(ins)
            return ins

        m = re.match(r"(while|loop|for|atomic)\b", t)
        if m:
            head, inline = split_header(t)
            body = self._inline_body(ln, child, inline)
            kw = m.group(1)
            if kw == "atomic":
                sub = []
                self._block(body, sub, loops=[], proc_end=None)
                for ins in sub:
                    if ins.kind in ("AWAIT", "DOWN", "CRIT", "NONCRIT", "ATOMIC"):
                        raise ILError(f"line {ins.line}: '{ins.src}' cannot be inside an atomic block")
                ins = add("ATOMIC", body=sub)
                inner = "; ".join(i.src for i in sub if not i.src.startswith("("))
                ins.src = f"atomic {{ {inner} }}"
                return
            if kw == "loop":
                start = len(code)
                ends = []
                self._block(body, code, loops + [ends], proc_end)
                add("JMP", target=start)
                for e in ends:
                    e.target = len(code)
                return
            if kw == "while":
                cond = Expr(head[len("while"):], ln.no)
                self._check_names(cond)
                start = len(code)
                jif = add("JIF", expr=cond, target=None)
                ends = []
                self._block(body, code, loops + [ends], proc_end)
                code.append(Instr("JMP", ln.no, "(loop back)", target=start))
                jif.target = len(code)
                for e in ends:
                    e.target = len(code)
                return
            fm = re.match(r"for\s+(\w+)\s+in\s+(.+?)\.\.(.+)$", head)
            if not fm:
                raise ILError(f"line {ln.no}: write 'for v in LOW..HIGH:' (inclusive)")
            v = fm.group(1)
            self._add_local(v)
            lo = Expr(fm.group(2), ln.no)
            hi = Expr(fm.group(3), ln.no)
            self._check_names(lo)
            self._check_names(hi)
            add("ASSIGN", target=v, idx=None, expr=lo)
            start = len(code)
            jif = add("JIF", expr=Expr(f"{v} <= ({fm.group(3)})", ln.no), target=None)
            ends = []
            self._block(body, code, loops + [ends], proc_end)
            code.append(Instr("ASSIGN", ln.no, f"{v} = {v} + 1", target=v, idx=None,
                              expr=Expr(f"{v} + 1", ln.no)))
            code.append(Instr("JMP", ln.no, "(loop back)", target=start))
            jif.target = len(code)
            for e in ends:
                e.target = len(code)
            return

        if child:
            raise ILError(f"line {ln.no}: unexpected indented block")

        if t in ("critical", "cs", "critical_section"):
            add("CRIT")
        elif t in ("noncritical", "ncs", "non_critical"):
            add("NONCRIT")
        elif t in ("skip", "pass", "nop") or re.match(r"sleep\b", t):
            add("JMP", target=len(code) + 1)
        elif t == "break":
            if not loops:
                raise ILError(f"line {ln.no}: 'break' outside a loop")
            loops[-1].append(add("JMP", target=None))
        elif t == "return":
            if proc_end is not None:
                proc_end.append(add("JMP", target=None))
            else:
                add("JMP", target=-1)   # resolved to END later
        elif re.match(r"await\b", t):
            e = Expr(t[len("await"):], ln.no)
            self._check_names(e)
            add("AWAIT", expr=e)
        elif re.match(r"assert\b", t):
            e = Expr(t[len("assert"):], ln.no)
            self._check_names(e)
            add("ASSERT", expr=e)
        elif re.match(r"print\b", t):
            arg = t[len("print"):].strip()
            if arg.startswith("(") and arg.endswith(")"):
                arg = arg[1:-1].strip()
            if not arg:
                raise ILError(f"line {ln.no}: print needs something to print")
            known = set(self.shared_names) | set(th["locals"]) | set(self.consts) | {th["param"]}
            if arg[0] in "\"'":
                add("PRINT", token=arg.strip("\"'"), expr=None)       # literal text
            elif re.fullmatch(r"[A-Za-z_]\w*", arg) and arg not in known:
                add("PRINT", token=arg, expr=None)                    # bare word: print A
            else:
                e = Expr(arg, ln.no)                                  # value of a variable/expression
                self._check_names(e)
                add("PRINT", token=None, expr=e)
        else:
            pm = re.match(r"(?:call\s+)?(\w+)\s*\((.*)\)$", t)
            if pm and pm.group(1) in self.procs:      # procedures win over sem keywords
                self._inline_proc(pm.group(1), self._split_commas(pm.group(2)), ln, code, loops)
                return
            sm = re.match(r"(\w+)\s*\.\s*(\w+)\s*\(\s*\)$", t) or None
            if sm and sm.group(2) in SEM_DOWN | SEM_UP:
                sem, op = sm.group(1), sm.group(2)
            else:
                sm = re.match(r"(\w+)\s*\(?\s*(\w+)\s*\)?$", t)
                op, sem = (sm.group(1), sm.group(2)) if sm else (None, None)
            if op in SEM_DOWN | SEM_UP and sem in self.sems:
                add("DOWN" if op in SEM_DOWN else "UP", sem=sem)
                return
            if op in SEM_DOWN | SEM_UP and sem is not None and sem not in self.procs:
                raise ILError(f"line {ln.no}: '{sem}' is not a declared semaphore")
            pm = re.match(r"(?:call\s+)?(\w+)\s*\((.*)\)$", t)
            if pm and pm.group(1) in self.procs:
                self._inline_proc(pm.group(1), self._split_commas(pm.group(2)), ln, code, loops)
                return
            am = re.match(r"(\w+)\s*(?:\[(.+)\])?\s*(\+\+|--|\+=|-=|=)\s*(.*)$", t)
            if am and not t.startswith("=="):
                name, idx, op, rhs = am.groups()
                if op == "++":
                    rhs, op = f"{name}{'[' + idx + ']' if idx else ''} + 1", "="
                elif op == "--":
                    rhs, op = f"{name}{'[' + idx + ']' if idx else ''} - 1", "="
                elif op in ("+=", "-="):
                    rhs = f"{name}{'[' + idx + ']' if idx else ''} {op[0]} ({rhs})"
                if name not in self.shared_names and name not in th["locals"]:
                    raise ILError(f"line {ln.no}: assignment to undeclared '{name}' "
                                  f"(declare it as shared or local)")
                e = Expr(rhs, ln.no)
                self._check_names(e)
                ie = None
                if idx is not None:
                    ie = Expr(idx, ln.no)
                    self._check_names(ie)
                add("ASSIGN", target=name, idx=ie, expr=e)
                return
            raise ILError(f"line {ln.no}: don't know how to run '{t}'")

    def _inline_proc(self, name, args, ln, code, loops):
        params, body = self.procs[name]
        if len(args) != len(params):
            raise ILError(f"line {ln.no}: {name} expects {len(params)} arguments")
        if getattr(self, "_inlining", 0) > 20:
            raise ILError(f"line {ln.no}: procedures cannot be recursive")
        self._inlining = getattr(self, "_inlining", 0) + 1
        base = body[0].indent if body else 0
        new = []
        for b in body:
            text = b.text
            for p, a in zip(params, args):
                text = re.sub(r"\b" + re.escape(p) + r"\b", a.strip(), text)
            new.append(Line(b.no, ln.indent + 4 + (b.indent - base), text))
        ends = []
        self._block(new, code, loops, proc_end=ends)
        for e in ends:
            e.target = len(code)
        self._inlining -= 1

    def _mark_local_only(self, th):
        end = len(th["code"]) - 1
        shared = set(self.shared_names)
        for ins in th["code"]:
            if ins.kind == "JMP" and ins.target == -1:
                ins.target = end
            if ins.kind == "JMP":
                ins.local_only = True
            elif ins.kind in ("JIF", "ASSIGN"):
                names = set(ins.expr.names)
                if ins.kind == "ASSIGN":
                    if ins.target in shared:
                        continue
                    if ins.idx is not None:
                        names |= ins.idx.names
                ins.local_only = not (names & shared)


# --------------------------------------------------------------------------
# Semantics
# --------------------------------------------------------------------------

RUN, HALTED, DONE = 0, 1, 2


class Machine:
    def __init__(self, prog, allow_halt=True, priority=None, bound=None, track_output=False,
                 max_output=12):
        self.p = prog
        self.allow_halt = allow_halt
        self.bound = bound
        self.track_output = track_output
        self.max_output = max_output
        self.sh_index = {n: k for k, n in enumerate(prog.shared_names)}
        self.prio = [1] * len(prog.threads)
        if priority:
            for k, th in enumerate(prog.threads):
                for key, val in priority.items():
                    if th["name"] == key or (th["name"].rstrip("0123456789") == key):
                        self.prio[k] = val
        self.truncated_outputs = False
        self.bound_hit = False

    # state = (shared tuple, threads tuple of (pc, locals, status), output tuple)
    def initial(self):
        threads = []
        for k, th in enumerate(self.p.threads):
            loc = tuple(th["local_init"])
            pc, loc, status, _ = self._fold(k, 0, loc, tuple(self.p.shared_init))
            threads.append((pc, loc, status))
        return (tuple(self.p.shared_init), tuple(threads), ())

    def env(self, k, shared, loc):
        th = self.p.threads[k]
        d = {"__builtins__": SAFE_BUILTINS}
        d.update(self.p.consts)
        d.update(zip(self.p.shared_names, shared))
        d.update(zip(th["locals"], loc))
        if th["param"]:
            d[th["param"]] = th["pval"]
        return d

    def eval(self, expr, k, shared, loc):
        try:
            return eval(expr.code, self.env(k, shared, loc))
        except Exception as e:
            raise RuntimeErrorIL(f"line {expr.line}: evaluating '{expr.text}' failed: {e}")

    def _assign(self, ins, k, shared, loc):
        th = self.p.threads[k]
        val = self.eval(ins.expr, k, shared, loc)
        if ins.target in th["locals"]:
            li = th["locals"].index(ins.target)
            if ins.idx is not None:
                raise RuntimeErrorIL(f"line {ins.line}: local arrays are not supported")
            loc = loc[:li] + (val,) + loc[li + 1:]
        else:
            si = self.sh_index[ins.target]
            if ins.idx is not None:
                arr = shared[si]
                i = self.eval(ins.idx, k, shared, loc)
                if not isinstance(arr, tuple) or not (0 <= i < len(arr)):
                    raise RuntimeErrorIL(f"line {ins.line}: index {i} out of range for '{ins.target}'")
                val = arr[:i] + (val,) + arr[i + 1:]
            shared = shared[:si] + (val,) + shared[si + 1:]
        return shared, loc

    def _fold(self, k, pc, loc, shared):
        """Run local-only instructions (they cannot affect other threads)."""
        code = self.p.threads[k]["code"]
        for _ in range(100000):
            ins = code[pc]
            if ins.kind == "END":
                return pc, loc, DONE, shared
            if not ins.local_only:
                return pc, loc, RUN, shared
            if ins.kind == "JMP":
                pc = ins.target
            elif ins.kind == "JIF":
                pc = pc + 1 if self.eval(ins.expr, k, shared, loc) else ins.target
            else:
                shared, loc = self._assign(ins, k, shared, loc)
                pc += 1
        raise RuntimeErrorIL(f"thread {self.p.threads[k]['name']} loops forever without "
                             f"touching shared variables")

    def enabled(self, state, k):
        shared, threads, _ = state
        pc, loc, status = threads[k]
        if status != RUN:
            return False
        ins = self.p.threads[k]["code"][pc]
        if ins.kind == "AWAIT":
            return bool(self.eval(ins.expr, k, shared, loc))
        if ins.kind == "DOWN":
            return shared[self.sh_index[ins.sem]] > 0
        return True

    def schedulable(self, state):
        en = [k for k in range(len(self.p.threads)) if self.enabled(state, k)]
        if en:
            top = max(self.prio[k] for k in en)
            en = [k for k in en if self.prio[k] == top]
        return en

    def at_critical(self, state, k):
        pc, _, status = state[1][k]
        return status == RUN and self.p.threads[k]["code"][pc].kind == "CRIT"

    def at_noncritical(self, state, k):
        pc, _, status = state[1][k]
        return status == HALTED or (status == RUN and
                                    self.p.threads[k]["code"][pc].kind == "NONCRIT")

    def step(self, state, k):
        """All successors when thread k takes one step: list of (state, info)."""
        shared, threads, out = state
        pc, loc, status = threads[k]
        ins = self.p.threads[k]["code"][pc]
        info = {"thread": k, "ins": ins, "cs": False, "print": None, "assert_fail": False}
        results = []

        def finish(shared2, loc2, pc2, out2=out, status2=RUN, info2=info):
            if status2 == RUN:
                pc2, loc2, status2, shared2 = self._fold(k, pc2, loc2, shared2)
            th2 = threads[:k] + ((pc2, loc2, status2),) + threads[k + 1:]
            results.append(((shared2, th2, out2), info2))

        kind = ins.kind
        if kind == "ASSIGN":
            s2, l2 = self._assign(ins, k, shared, loc)
            finish(s2, l2, pc + 1)
        elif kind == "AWAIT":
            finish(shared, loc, pc + 1)
        elif kind == "JIF":
            nxt = pc + 1 if self.eval(ins.expr, k, shared, loc) else ins.target
            finish(shared, loc, nxt)
        elif kind == "JMP":
            finish(shared, loc, ins.target)
        elif kind == "CRIT":
            info["cs"] = True
            finish(shared, loc, pc + 1)
        elif kind == "NONCRIT":
            finish(shared, loc, pc + 1)
            if self.allow_halt:
                info_h = dict(info, halt=True)
                th2 = threads[:k] + ((pc, loc, HALTED),) + threads[k + 1:]
                results.append(((shared, th2, out), info_h))
        elif kind == "PRINT":
            token = self.print_token(ins, k, shared, loc)
            info["print"] = token
            out2 = out
            if self.track_output:
                if len(out) >= self.max_output:
                    self.truncated_outputs = True
                    return []
                out2 = out + (token,)
            finish(shared, loc, pc + 1, out2)
        elif kind in ("DOWN", "UP"):
            si = self.sh_index[ins.sem]
            delta = -1 if kind == "DOWN" else 1
            s2 = shared[:si] + (shared[si] + delta,) + shared[si + 1:]
            finish(s2, loc, pc + 1)
        elif kind == "ASSERT":
            if not self.eval(ins.expr, k, shared, loc):
                info["assert_fail"] = True
            finish(shared, loc, pc + 1)
        elif kind == "ATOMIC":
            s2, l2, out2, prints = self._run_atomic(ins.body, k, shared, loc, out)
            info["print"] = " ".join(prints) if prints else None
            finish(s2, l2, pc + 1, out2)
        else:
            raise RuntimeErrorIL(f"cannot step '{kind}'")
        if self.bound is not None:
            kept = []
            for st, inf in results:
                if self._exceeds(st):
                    self.bound_hit = True
                else:
                    kept.append((st, inf))
            results = kept
        return results

    def print_token(self, ins, k, shared, loc):
        if ins.expr is None:
            return ins.token
        return fmt_val(self.eval(ins.expr, k, shared, loc))

    def _run_atomic(self, body, k, shared, loc, out):
        pc, prints = 0, []
        for _ in range(100000):
            if pc >= len(body):
                return shared, loc, out, prints
            ins = body[pc]
            if ins.kind == "ASSIGN":
                shared, loc = self._assign(ins, k, shared, loc)
                pc += 1
            elif ins.kind == "JIF":
                pc = pc + 1 if self.eval(ins.expr, k, shared, loc) else ins.target
            elif ins.kind == "JMP":
                pc = ins.target if ins.target is not None and ins.target >= 0 else len(body)
            elif ins.kind == "PRINT":
                token = self.print_token(ins, k, shared, loc)
                prints.append(token)
                if self.track_output:
                    out = out + (token,)
                pc += 1
            elif ins.kind == "UP":
                si = self.sh_index[ins.sem]
                shared = shared[:si] + (shared[si] + 1,) + shared[si + 1:]
                pc += 1
            elif ins.kind == "ASSERT":
                if not self.eval(ins.expr, k, shared, loc):
                    raise RuntimeErrorIL(f"line {ins.line}: assertion failed inside atomic block")
                pc += 1
            else:
                raise RuntimeErrorIL(f"line {ins.line}: '{ins.src}' not allowed in atomic")
        raise RuntimeErrorIL("atomic block loops forever")

    def _exceeds(self, st):
        def big(v):
            if isinstance(v, tuple):
                return any(big(x) for x in v)
            return isinstance(v, int) and not isinstance(v, bool) and abs(v) > self.bound
        return big(st[0]) or any(big(t[1]) for t in st[1])


class RuntimeErrorIL(Exception):
    pass


# --------------------------------------------------------------------------
# Exploration
# --------------------------------------------------------------------------

class Graph:
    def __init__(self, m, max_states):
        self.m = m
        init = m.initial()
        self.states = [init]
        self.index = {init: 0}
        self.parent = [None]           # (parent_id, info)
        self.edges = [[]]              # list of (dst, info)
        self.sched = []                # schedulable threads per state
        self.complete = True
        q = deque([0])
        while q:
            sid = q.popleft()
            st = self.states[sid]
            sc = m.schedulable(st)
            while len(self.sched) <= sid:
                self.sched.append(None)
            self.sched[sid] = sc
            for k in sc:
                for nst, info in m.step(st, k):
                    nid = self.index.get(nst)
                    if nid is None:
                        if len(self.states) >= max_states:
                            self.complete = False
                            continue
                        nid = len(self.states)
                        self.index[nst] = nid
                        self.states.append(nst)
                        self.parent.append((sid, info))
                        self.edges.append([])
                        q.append(nid)
                    self.edges[sid].append((nid, info))

    def path_to(self, sid):
        steps = []
        while self.parent[sid] is not None:
            pid, info = self.parent[sid]
            steps.append((pid, info, sid))
            sid = pid
        return list(reversed(steps))

    def sccs(self):
        """Tarjan's algorithm (iterative). Returns list of lists of state ids."""
        n = len(self.states)
        index, low, on, stack, out = [None] * n, [0] * n, [False] * n, [], []
        counter = 0
        for root in range(n):
            if index[root] is not None:
                continue
            work = [(root, 0)]
            while work:
                v, ei = work.pop()
                if ei == 0:
                    index[v] = low[v] = counter
                    counter += 1
                    stack.append(v)
                    on[v] = True
                recurse = False
                edges = self.edges[v]
                while ei < len(edges):
                    w = edges[ei][0]
                    ei += 1
                    if index[w] is None:
                        work.append((v, ei))
                        work.append((w, 0))
                        recurse = True
                        break
                    if on[w]:
                        low[v] = min(low[v], index[w])
                if recurse:
                    continue
                if low[v] == index[v]:
                    comp = []
                    while True:
                        w = stack.pop()
                        on[w] = False
                        comp.append(w)
                        if w == v:
                            break
                    out.append(comp)
                if work:
                    u = work[-1][0]
                    low[u] = min(low[u], low[v])
        return out


def tarjan(nodes, succ):
    """SCCs of the subgraph on `nodes` (a set); succ(v) yields neighbours."""
    index, low, on, stack, out = {}, {}, set(), [], []
    counter = 0
    for root in nodes:
        if root in index:
            continue
        work = [(root, iter(list(succ(root))))]
        index[root] = low[root] = counter
        counter += 1
        stack.append(root)
        on.add(root)
        while work:
            v, it = work[-1]
            advanced = False
            for w in it:
                if w not in nodes:
                    continue
                if w not in index:
                    index[w] = low[w] = counter
                    counter += 1
                    stack.append(w)
                    on.add(w)
                    work.append((w, iter(list(succ(w)))))
                    advanced = True
                    break
                if w in on:
                    low[v] = min(low[v], index[w])
            if advanced:
                continue
            work.pop()
            if work:
                u = work[-1][0]
                low[u] = min(low[u], low[v])
            if low[v] == index[v]:
                comp = []
                while True:
                    w = stack.pop()
                    on.discard(w)
                    comp.append(w)
                    if w == v:
                        break
                out.append(comp)
    return out


def fair_cycles(g, m, state_ok=None, edge_ok=None):
    """Yield (scc_set, movers, cs_threads, internal_edges) for every SCC that
    allows a weakly-fair infinite run (each thread that is continuously
    schedulable eventually moves).

    state_ok / edge_ok restrict the search to a subgraph, e.g. "no critical
    section step" (livelock) or "thread t is trying" (starvation of t)."""
    nthreads = len(m.p.threads)
    nodes = {v for v in range(len(g.states)) if state_ok is None or state_ok(v)}

    def succ(v):
        for (w, info) in g.edges[v]:
            if edge_ok is None or edge_ok(info):
                yield w

    for comp in tarjan(nodes, succ):
        cs = set(comp)
        internal = [(v, w, info) for v in comp for (w, info) in g.edges[v]
                    if w in cs and (edge_ok is None or edge_ok(info))]
        if not internal:
            continue
        movers = {info["thread"] for _, _, info in internal}
        fair = True
        for u in range(nthreads):
            if u in movers:
                continue
            if all(u in g.sched[v] for v in comp):
                fair = False      # u could run in every state but never does
                break
        if not fair:
            continue
        cs_threads = {info["thread"] for _, _, info in internal if info["cs"]}
        yield cs, movers, cs_threads, internal


def cycle_in(g, comp, start, must_threads=(), edge_ok=None):
    """A cycle start -> ... -> start inside the SCC (BFS), preferring to include
    a step by each thread in must_threads."""
    target_order = list(must_threads)
    path, cur = [], start
    visited_threads = set()
    for t in target_order + [None]:
        if t is not None and t in visited_threads:
            continue
        # BFS from cur to a state from which t steps (or back to start)
        prev = {cur: None}
        q = deque([cur])
        found = None
        while q:
            v = q.popleft()
            for (w, info) in g.edges[v]:
                if w not in comp or (edge_ok is not None and not edge_ok(info)):
                    continue
                if (t is not None and info["thread"] == t) or (t is None and w == start):
                    found = (v, w, info)
                    break
                if w not in prev:
                    prev[w] = (v, info)
                    q.append(w)
            if found:
                break
        if not found:
            continue
        v, w, info = found
        seg = []
        x = v
        while prev[x] is not None:
            pv, pinfo = prev[x]
            seg.append((pv, pinfo, x))
            x = pv
        seg.reverse()
        seg.append((v, info, w))
        for (_, inf, _) in seg:
            visited_threads.add(inf["thread"])
        path += seg
        cur = w
        if t is None:
            break
    return path


# --------------------------------------------------------------------------
# Reporting
# --------------------------------------------------------------------------

def fmt_val(v):
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, tuple):
        return "[" + ",".join(fmt_val(x) for x in v) + "]"
    return str(v)


def describe_state(m, st):
    shared, threads, _ = st
    parts = [f"{n}={fmt_val(v)}" for n, v in zip(m.p.shared_names, shared)]
    return ", ".join(parts)


def thread_pos(m, st, k):
    pc, loc, status = st[1][k]
    th = m.p.threads[k]
    if status == DONE:
        return "finished"
    if status == HALTED:
        return "stopped in its non-critical section"
    ins = th["code"][pc]
    return f"at line {ins.line}: {ins.src}"


def print_steps(g, m, steps, start_no=1):
    no = start_no
    for (pid, info, sid) in steps:
        before, after = g.states[pid], g.states[sid]
        name = m.p.threads[info["thread"]]["name"]
        ins = info["ins"]
        what = ins.src if ins.kind != "NONCRIT" else "noncritical"
        if ins.kind == "CRIT":
            what = "critical   (finishes, leaves the CS)"
        if info.get("halt"):
            what = "noncritical  (and stays there forever)"
        changes = [f"{n}={fmt_val(b)}" for n, a, b in zip(m.p.shared_names, before[0], after[0]) if a != b]
        th = m.p.threads[info["thread"]]
        lch = [f"{n}={fmt_val(b)}" for n, a, b in zip(th["locals"], before[1][info["thread"]][1],
                                                      after[1][info["thread"]][1]) if a != b]
        extra = ""
        if changes or lch:
            extra = "   -> " + ", ".join(changes + [f"({c})" for c in lch])
        if info["print"]:
            extra += f"   prints {info['print']}"
        if m.at_critical(after, info["thread"]) and not m.at_critical(before, info["thread"]):
            extra += "   ==> enters CS"
        print(f"  {no:3d}. {name:<6} line {ins.line:<3} {what}{extra}")
        no += 1
    return no


def report_protocol(prog, args):
    m = Machine(prog, allow_halt=not args.no_halt, priority=args.priority, bound=args.bound)
    g = Graph(m, args.max_states)
    nthreads = len(prog.threads)
    has_cs = any(ins.kind == "CRIT" for th in prog.threads for ins in th["code"])
    has_ncs = any(ins.kind == "NONCRIT" for th in prog.threads for ins in th["code"])
    ntrans = sum(len(e) for e in g.edges)
    names = ", ".join(t["name"] for t in prog.threads)
    print(f"{prog.filename}: threads {names}; explored {len(g.states)} states, {ntrans} transitions")
    if not g.complete:
        print(f"  WARNING: stopped at --max-states {args.max_states}; results below are partial")
    if m.bound_hit:
        print(f"  NOTE: states with values beyond --bound {args.bound} were cut off")
    if m.allow_halt and has_ncs:
        print("  (a thread may stop forever in `noncritical`, as the lecture's rules allow;"
              " use --no-halt to forbid)")
    if args.priority:
        print(f"  scheduler: strict priority {args.priority} (single core)")
    verdict = {}

    # 1. mutual exclusion / assertions
    me_bad = None
    assert_bad = None
    for sid, st in enumerate(g.states):
        if me_bad is None and has_cs and sum(m.at_critical(st, k) for k in range(nthreads)) >= 2:
            me_bad = sid
    for sid, edges in enumerate(g.edges):
        for (w, info) in edges:
            if info["assert_fail"] and assert_bad is None:
                assert_bad = (sid, info, w)
    print()
    if has_cs:
        if me_bad is None:
            print("[OK]    Mutual exclusion: never two threads in the critical section")
            verdict["mutex"] = "ok"
        else:
            st = g.states[me_bad]
            inside = [m.p.threads[k]["name"] for k in range(nthreads) if m.at_critical(st, k)]
            print(f"[FAIL]  Mutual exclusion VIOLATED: {' and '.join(inside)} are both in the critical section")
            print("        Shortest schedule that gets there:")
            print_steps(g, m, g.path_to(me_bad))
            print(f"        state: {describe_state(m, st)}")
            verdict["mutex"] = "violated"
    if assert_bad is not None:
        sid, info, w = assert_bad
        print(f"[FAIL]  Assertion failed: line {info['ins'].line}: {info['ins'].src}")
        print_steps(g, m, g.path_to(sid) + [(sid, info, w)])
        verdict["assert"] = "violated"
    elif any(ins.kind == "ASSERT" for th in prog.threads for ins in th["code"]):
        print("[OK]    Every assertion holds")
        verdict["assert"] = "ok"

    # 2. deadlock / blocked
    dead = blocked = None
    for sid, st in enumerate(g.states):
        if g.sched[sid]:
            continue
        statuses = [t[2] for t in st[1]]
        if all(s != RUN for s in statuses):
            continue                      # everyone finished or halted
        if any(s == HALTED for s in statuses):
            blocked = blocked if blocked is not None else sid
        else:
            dead = dead if dead is not None else sid
    if dead is None:
        print("[OK]    No deadlock")
        verdict["deadlock"] = "none"
    else:
        st = g.states[dead]
        print("[FAIL]  DEADLOCK: every unfinished thread is waiting and nobody can move")
        print_steps(g, m, g.path_to(dead))
        for k in range(nthreads):
            print(f"        {m.p.threads[k]['name']} is {thread_pos(m, st, k)}")
        print(f"        state: {describe_state(m, st)}")
        verdict["deadlock"] = "found"
    if has_ncs and m.allow_halt:
        if blocked is None:
            print("[OK]    No thread is blocked when another stays in its non-critical section")
            verdict["blocked"] = "none"
        else:
            st = g.states[blocked]
            waiting = [m.p.threads[k]["name"] for k in range(nthreads) if st[1][k][2] == RUN]
            stopped = [m.p.threads[k]["name"] for k in range(nthreads) if st[1][k][2] == HALTED]
            print(f"[FAIL]  BLOCKED FOREVER: {', '.join(waiting)} can never enter because "
                  f"{', '.join(stopped)} stopped in its non-critical section")
            print("        (fails 'success in the absence of contention')")
            print_steps(g, m, g.path_to(blocked))
            verdict["blocked"] = "found"

    # 3. livelock / starvation (fair cycles)
    if has_cs:
        def is_trying(v, k):
            st = g.states[v]
            return st[1][k][2] == RUN and not m.at_noncritical(st, k) and not m.at_critical(st, k)

        live = starve = None
        # livelock: a fair loop with no critical-section step at all, in which
        # some thread is trying the whole time
        for comp, movers, _, _ in fair_cycles(g, m, edge_ok=lambda info: not info["cs"]):
            trying = [k for k in range(nthreads) if all(is_trying(v, k) for v in comp)]
            if trying:
                live = (comp, movers, trying)
                break
        # starvation of thread k: a fair loop in which k is trying throughout
        # (so it never gets in) while some other thread does enter
        for k in range(nthreads):
            if starve is not None:
                break
            for comp, movers, cs_threads, _ in fair_cycles(g, m, state_ok=lambda v, k=k: is_trying(v, k)):
                if cs_threads:
                    starve = (comp, movers, [k])
                    break
        for label, item, key in (("LIVELOCK", live, "livelock"), ("STARVATION", starve, "starvation")):
            if item is None:
                print(f"[OK]    No {label.lower()} (under a fair scheduler)")
                verdict[key] = "none"
                continue
            comp, movers, trying = item
            entry = min(comp)
            victims = ", ".join(m.p.threads[k]["name"] for k in trying)
            if key == "livelock":
                print(f"[FAIL]  LIVELOCK: threads keep running forever but nobody enters the critical section"
                      f" ({victims} keep trying)")
            else:
                print(f"[FAIL]  STARVATION: {victims} can be overtaken forever while others keep entering")
            print("        Reach the loop with:")
            no = print_steps(g, m, g.path_to(entry))
            print("        ...then repeat forever:")
            eok = (lambda info: not info["cs"]) if key == "livelock" else None
            print_steps(g, m, cycle_in(g, comp, entry, sorted(movers), eok), no)
            verdict[key] = "found"
    return g, m, verdict


def report_outputs(prog, args):
    m = Machine(prog, allow_halt=False, priority=args.priority, bound=args.bound,
                track_output=True, max_output=args.max_output)
    g = Graph(m, args.max_states)
    finals = {}
    for sid, st in enumerate(g.states):
        if all(t[2] == DONE for t in st[1]):
            finals.setdefault(st[2], sid)
    stuck = [sid for sid, st in enumerate(g.states)
             if not g.sched[sid] and any(t[2] == RUN for t in st[1])]
    print(f"{prog.filename}: explored {len(g.states)} states")
    if not g.complete:
        print("  WARNING: state limit reached; the list may be incomplete")
    if m.truncated_outputs:
        print(f"  NOTE: runs printing more than {args.max_output} items were cut off (--max-output)")
    if args.why is not None:
        want = tuple(args.why.split())
        sid = finals.get(want)
        if sid is None:
            print(f"\nOutput '{args.why}' with every thread finishing is IMPOSSIBLE.")
            close = [o for o in finals if o[:len(want)] == want or want[:len(o)] == o]
            if close:
                print("  Possible outputs that start the same way: " +
                      "; ".join(" ".join(o) or "(nothing)" for o in sorted(close)[:8]))
        else:
            print(f"\nOutput '{args.why}' IS possible. One schedule:")
            print_steps(g, m, g.path_to(sid))
        return
    print(f"\nPossible outputs when every thread finishes ({len(finals)}):")
    ordered = sorted(finals, key=lambda o: (len(o), o))
    for out in ordered[:args.show]:
        print("   ", " ".join(out) if out else "(prints nothing)")
    if len(ordered) > args.show:
        print(f"    ... and {len(ordered) - args.show} more (use --show N, or --why \"...\" to test one)")
    if stuck:
        st = g.states[stuck[0]]
        print(f"\nSome runs get STUCK (threads wait forever), e.g. after output "
              f"'{' '.join(st[2]) or '(nothing)'}':")
        for k in range(len(prog.threads)):
            print(f"    {prog.threads[k]['name']} is {thread_pos(m, st, k)}")
    # non-terminating behaviour (explore again without output in the state)
    m2 = Machine(prog, allow_halt=False, priority=args.priority, bound=args.bound)
    g2 = Graph(m2, args.max_states)
    loops = list(fair_cycles(g2, m2))
    if loops:
        print("\nThe program can also run FOREVER (fair schedules). Example loops:")
        shown = 0
        for comp, movers, _, internal in loops:
            entry = min(comp)
            cyc = cycle_in(g2, comp, entry, sorted(movers))
            pre = [i["print"] for (_, i, _) in g2.path_to(entry) if i["print"]]
            rep = [i["print"] for (_, i, _) in cyc if i["print"]]
            print(f"    {' '.join(pre)} ( {' '.join(rep) or 'no output'} ) repeated forever")
            shown += 1
            if shown >= 5:
                break
        print("  Use --infinite \"B A\" to ask whether a specific repeating output is possible.")


def report_infinite(prog, args):
    """Is there a fair infinite run whose whole output is (pattern)^omega?"""
    pat = args.infinite.split()
    m = Machine(prog, allow_halt=False, priority=args.priority, bound=args.bound)
    # product graph: (state, position in pattern)
    init = (m.initial(), 0)
    index = {init: 0}
    nodes = [init]
    edges = [[]]
    sched = []
    parent = [None]
    q = deque([0])
    while q:
        nid = q.popleft()
        st, pos = nodes[nid]
        sc = m.schedulable(st)
        sched.append((nid, sc))
        for k in sc:
            for nst, info in m.step(st, k):
                if info["print"]:
                    toks = info["print"].split()
                    p, ok = pos, True
                    for tkn in toks:
                        if tkn != pat[p]:
                            ok = False
                            break
                        p = (p + 1) % len(pat)
                    if not ok:
                        continue
                    npos = p
                else:
                    npos = pos
                node = (nst, npos)
                tid = index.get(node)
                if tid is None:
                    if len(nodes) >= args.max_states:
                        continue
                    tid = len(nodes)
                    index[node] = tid
                    nodes.append(node)
                    edges.append([])
                    parent.append((nid, info))
                    q.append(tid)
                edges[nid].append((tid, info))

    class PG:
        pass
    pg = PG()
    pg.states = [n[0] for n in nodes]
    pg.edges = edges
    pg.sched = [None] * len(nodes)
    for nid, sc in sched:
        pg.sched[nid] = sc
    pg.sccs = lambda: Graph.sccs(pg)
    pg.path_to = lambda sid: Graph.path_to(pg, sid)
    pg.parent = parent
    for comp, movers, _, internal in fair_cycles(pg, m):
        if not any(info["print"] for _, _, info in internal):
            continue
        entry = min(comp)
        print(f"Infinite output ({' '.join(pat)}) repeated forever IS possible (fair schedule):")
        no = print_steps(pg, m, pg.path_to(entry))
        print("  ...then repeat forever:")
        print_steps(pg, m, cycle_in(pg, comp, entry, sorted(movers)), no)
        return
    print(f"Infinite output ({' '.join(pat)}) repeated forever is IMPOSSIBLE under a fair scheduler.")


def interactive(prog, args):
    m = Machine(prog, allow_halt=not args.no_halt, priority=args.priority, track_output=True,
                max_output=10 ** 6)
    st = m.initial()
    history = []
    print("Interactive mode: type a thread number to run its next step, 'u' to undo, 'q' to quit.")
    while True:
        print("\n  shared:", describe_state(m, st) or "(none)")
        if st[2]:
            print("  output so far:", " ".join(st[2]))
        sc = m.schedulable(st)
        for k, th in enumerate(prog.threads):
            mark = "runnable" if k in sc else ("waiting" if st[1][k][2] == RUN else "")
            loc = ", ".join(f"{n}={fmt_val(v)}" for n, v in zip(th["locals"], st[1][k][1]))
            print(f"  [{k}] {th['name']:<6} {thread_pos(m, st, k):<45} {mark} {loc}")
        if sum(m.at_critical(st, k) for k in range(len(prog.threads))) >= 2:
            print("  !!! two threads are in the critical section !!!")
        if not sc:
            print("  Nobody can move." + ("" if all(t[2] != RUN for t in st[1]) else " DEADLOCK/BLOCKED."))
        try:
            cmd = input("> ").strip()
        except EOFError:
            return
        if cmd == "q":
            return
        if cmd == "u" and history:
            st = history.pop()
            continue
        if cmd.isdigit() and int(cmd) in sc:
            succ = m.step(st, int(cmd))
            choice = 0
            if len(succ) > 1:
                print("  0 = continue, 1 = stay in the non-critical section forever")
                c = input("  choice> ").strip()
                choice = 1 if c == "1" else 0
            history.append(st)
            st = succ[choice][0]
        else:
            print("  not a runnable thread")


def parse_priority(s):
    if not s:
        return None
    out = {}
    for part in s.split(","):
        k, v = part.split("=")
        out[k.strip()] = int(v)
    return out


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("file")
    ap.add_argument("--outputs", action="store_true")
    ap.add_argument("--why")
    ap.add_argument("--infinite")
    ap.add_argument("--step", action="store_true")
    ap.add_argument("--no-halt", action="store_true")
    ap.add_argument("--priority", type=parse_priority)
    ap.add_argument("--bound", type=int)
    ap.add_argument("--max-states", type=int, default=500000)
    ap.add_argument("--max-output", type=int, default=12)
    ap.add_argument("--verdict", action="store_true")
    ap.add_argument("--show", type=int, default=30)
    args = ap.parse_args(argv)
    try:
        with open(args.file) as f:
            prog = Program(f.read(), args.file)
        if not prog.threads:
            raise ILError("no threads defined")
        if args.step:
            interactive(prog, args)
        elif args.infinite:
            report_infinite(prog, args)
        elif args.outputs or args.why is not None:
            report_outputs(prog, args)
        else:
            _, _, verdict = report_protocol(prog, args)
            if args.verdict:
                print("VERDICT " + " ".join(f"{k}={v}" for k, v in sorted(verdict.items())))
    except (ILError, RuntimeErrorIL) as e:
        print(f"error: {e}", file=sys.stderr)
        return 2
    except FileNotFoundError:
        print(f"error: no such file {args.file}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
