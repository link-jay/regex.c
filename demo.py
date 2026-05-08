#!venv/bin/python3

epsilon = ""

Syms = [epsilon, "&", "|", "*", "+", "(", ")"]
Syms_ucon = [epsilon, "&", "|"]
Sym_Priory = {
    epsilon: 0,
    "&": 1,
    "|": 2,
    "*": 3,
    "+": 3,
    # NOTE: 括号语法并没有实现
    "(": 0,
    ")": 9
}

class NfaState:
    def __init__(self, is_end: bool):
        self.is_end: bool = is_end
        self.next_state: dict[str, set[NfaState]] = {}

    def goto_next_state(self, ch: str, to: NfaState):
        self.next_state.setdefault(ch, set()).add(to)

class NFA:
    def __init__(self, start: NfaState, end: NfaState):
        self.start = start
        self.end = end

class NfaParser:
    @staticmethod
    def create_new_nfa(ch: str) -> NFA:
        a = NfaState(False)
        b = NfaState(True)
        a.goto_next_state(ch, b)
        return NFA(a, b)

    @staticmethod
    def concat(start: NFA, end: NFA) -> NFA:
        start.end.goto_next_state(epsilon, end.start)
        start.end.is_end = False
        return NFA(start.start, end.end)

    @staticmethod
    def union(left: NFA, right: NFA) -> NFA:
        start = NfaState(False)
        end = NfaState(True)
        start.goto_next_state(epsilon, left.start)
        start.goto_next_state(epsilon, right.start)
        left.end.goto_next_state(epsilon, end)
        left.end.is_end = False
        right.end.goto_next_state(epsilon, end)
        right.end.is_end = False
        return NFA(start, end)

    @staticmethod
    def closure(a: NFA) -> NFA:
        start = NfaState(False)
        end = NfaState(True)
        start.goto_next_state(epsilon, a.start)
        a.end.goto_next_state(epsilon, end)
        a.end.is_end = False
        a.end.goto_next_state(epsilon, a.start)
        start.goto_next_state(epsilon, end)
        return NFA(start, end)

    @staticmethod
    def plus(a: NFA) -> NFA:
        return NFA(a.start, NfaParser.closure(a).end)

def parse_src(src: str) -> str:
    dst: list = [""]
    for i in range(len(src)):
        if src[i] == "(":
            dst.append("&")
            dst.append(src[i])
        elif src[i] == ")":
            dst.append(src[i])
            dst.append("&")
        elif src[i] == "*" or src[i] == "+":
            if i < len(src) - 1:
                dst.append(src[i])
                dst.append("&")
            else:
                dst.append(src[i])
        elif src[i] in Syms_ucon or dst[-1] in Syms_ucon:
            dst.append(src[i])
        else:
            dst.append("&")
            dst.append(src[i])
    return "".join(dst)

def compile_regex_simple(flag: str, sym_stack: list[str], nfa_stack: list[NFA]):
    sym = sym_stack.pop()
    while sym != flag:
        match sym:
            case "&":
                b = nfa_stack.pop()
                a = nfa_stack.pop()
                nfa_stack.append(NfaParser.concat(a, b))
            case "|":
                b = nfa_stack.pop()
                a = nfa_stack.pop()
                nfa_stack.append(NfaParser.union(a, b))
            case "*":
                a = nfa_stack.pop()
                nfa_stack.append(NfaParser.closure(a))
            case "+":
                a = nfa_stack.pop()
                nfa_stack.append(NfaParser.plus(a))
        sym = sym_stack.pop()

def compile_regex(dst: str) -> NFA:
    sym_stack: list[str] = [epsilon]
    nfa_stack: list[NFA] = []
    for i in dst:
        if i not in Syms:
            nfa_stack.append(NfaParser.create_new_nfa(i))
        else:
            prev_sym = sym_stack.pop()
            # if i == "(": ...
            # elif i == ")":
            #     compile_regex_simple("(", sym_stack, nfa_stack)
            if Sym_Priory[prev_sym] >= Sym_Priory[i]:
                match prev_sym:
                    case "&":
                        b = nfa_stack.pop()
                        a = nfa_stack.pop()
                        nfa_stack.append(NfaParser.concat(a, b))
                    case "|":
                        b = nfa_stack.pop()
                        a = nfa_stack.pop()
                        nfa_stack.append(NfaParser.union(a, b))
                    case "*":
                        a = nfa_stack.pop()
                        nfa_stack.append(NfaParser.closure(a))
                    case "+":
                        a = nfa_stack.pop()
                        nfa_stack.append(NfaParser.plus(a))
            else:
                sym_stack.append(prev_sym)
            sym_stack.append(i)
    compile_regex_simple(epsilon, sym_stack, nfa_stack)
    return nfa_stack.pop()

def epsilon_closure(state: NfaState) -> set[NfaState]:
    stack: list[NfaState] = [state]
    closure: set[NfaState] = {state,}
    while stack != []:
        for i in stack.pop().next_state.get(epsilon, {}):
            if i not in closure:
                closure.add(i)
                stack.append(i)
    return closure

def epsilon_closure_set(states: set[NfaState]) -> set[NfaState]:
    closure: set[NfaState] = set()
    for st in states:
        closure.update(epsilon_closure(st))
    return closure

def nfa_match(nfa: NFA, src: str) -> bool:
    current_states: set[NfaState] = epsilon_closure(nfa.start)
    for c in src:
        next_states: set[NfaState] = set()
        for st in current_states:
            next_states.update(st.next_state.get(c, {}))
            next_states.update(st.next_state.get(".", {}))
        current_states = epsilon_closure_set(next_states)
        if not current_states:
            return False
    return any(st.is_end for st in current_states)
    


assert nfa_match(compile_regex(parse_src("abc")), "abc")
assert nfa_match(compile_regex(parse_src("abc|d")), "abd")
assert nfa_match(compile_regex(parse_src("a*bc")), "aaabc")
assert nfa_match(compile_regex(parse_src("a*bc")), "bc")
assert nfa_match(compile_regex(parse_src("a+bc")), "aaabc")
assert not nfa_match(compile_regex(parse_src("a+bc")), "bc")
assert nfa_match(compile_regex(parse_src(".*bc")), "bbc")
assert nfa_match(compile_regex(parse_src(".*")), "bbc")
assert nfa_match(compile_regex(parse_src(".*")), "")
assert not nfa_match(compile_regex(parse_src(".+")), "")
