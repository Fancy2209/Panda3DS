#if defined(PANDA3DS_DYNAPICA_SUPPORTED) && defined(PANDA3DS_X64_HOST)
#include "PICA/dynapica/shader_rec_emitter_x64.hpp"

#include <algorithm>
#include <cstddef>

using namespace Xbyak;
using namespace Xbyak::util;

// Register that points to PICA state
static constexpr Reg64 statePointer = rbp;
static constexpr Xmm scratch1 = xmm0;
static constexpr Xmm scratch2 = xmm1;
static constexpr Xmm scratch3 = xmm2;

void ShaderEmitter::compile(const PICAShader& shaderUnit) {
	// Emit prologue first
	align(16);
	prologueCb = getCurr<PrologueCallback>();

	// We assume arg1 contains the pointer to the PICA state and arg2 a pointer to the code for the entrypoint
	push(statePointer); // Back up state pointer to stack. This also aligns rsp to 16 bytes for calls
	mov(statePointer, (uintptr_t)&shaderUnit); // Set state pointer to the proper pointer

	// If we add integer register allocations they should be pushed here, and the rsp should be properly fixed up
	// However most of the PICA is floating point so yeah

	// Allocate shadow stack on Windows
	if constexpr (isWindows()) {
		sub(rsp, 32);
	}
	// Tail call to shader code entrypoint
	jmp(arg2);
	align(16);
	// Scan the shader code for call instructions and add them to the list of possible return PCs. We need to do this because the PICA callstack works
	// Pretty weirdly
	scanForCalls(shaderUnit);

	// Compile every instruction in the shader
	// This sounds horrible but the PICA instruction memory is tiny, and most of the time it's padded wtih nops that compile to nothing
	recompilerPC = 0;
	compileUntil(shaderUnit, PICAShader::maxInstructionCount);
}

void ShaderEmitter::scanForCalls(const PICAShader& shaderUnit) {
	returnPCs.clear();

	for (u32 i = 0; i < PICAShader::maxInstructionCount; i++) {
		const u32 instruction = shaderUnit.loadedShader[i];
		if (isCall(instruction)) {
			const u32 num = instruction & 0xff; // Num of instructions to execute
			const u32 dest = (instruction >> 10) & 0xfff; // Starting subroutine address
			const u32 returnPC = num + dest; // Add them to get the return PC

			returnPCs.push_back(returnPC);
		}
	}
}

void ShaderEmitter::compileUntil(const PICAShader& shaderUnit, u32 end) {
	while (recompilerPC < end) {
		compileInstruction(shaderUnit);
	}
}

void ShaderEmitter::compileInstruction(const PICAShader& shaderUnit) {
	// Write current location to label for this instruction
	L(instructionLabels[recompilerPC]);
	// Fetch instruction and inc PC
	const u32 instruction = shaderUnit.loadedShader[recompilerPC++];
	const u32 opcode = instruction >> 26;

	switch (opcode) {
		case ShaderOpcodes::MOV: recMOV(shaderUnit, instruction); break;
		default:
			Helpers::panic("ShaderJIT: Unimplemented PICA opcode %X", opcode);
	}
}

void ShaderEmitter::loadRegister(Xmm dest, const PICAShader& shader, u32 srcReg, u32 index) {

}

void ShaderEmitter::recMOV(const PICAShader& shader, u32 instruction) {
	/*
	const u32 operandDescriptor = shader.operandDescriptors[instruction & 0x7f];
	u32 src = (instruction >> 12) & 0x7f;
	const u32 idx = (instruction >> 19) & 3;
	const u32 dest = (instruction >> 21) & 0x1f;

	src = getIndexedSource(src, idx);
	vec4f srcVector = getSourceSwizzled<1>(src, operandDescriptor);
	vec4f& destVector = getDest(dest);

	u32 componentMask = operandDescriptor & 0xf;
	*/
}

#endif