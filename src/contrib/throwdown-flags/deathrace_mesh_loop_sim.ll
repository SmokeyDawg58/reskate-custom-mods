;
; Input signature:
;
; Name                 Index   Mask Register SysValue  Format   Used
; -------------------- ----- ------ -------- -------- ------- ------
; no parameters
;
; Output signature:
;
; Name                 Index   Mask Register SysValue  Format   Used
; -------------------- ----- ------ -------- -------- ------- ------
; no parameters
; shader debug name: 1783a397b6849efbc21d3652a36cb4c2.pdb
; shader hash: 1783a397b6849efbc21d3652a36cb4c2
;
; Pipeline Runtime Information: 
;
;PSVRuntimeInfo:
; Compute Shader
; NumThreads=(64,1,1)
; MinimumExpectedWaveLaneCount: 0
; MaximumExpectedWaveLaneCount: 4294967295
; UsesViewID: false
; SigInputElements: 0
; SigOutputElements: 0
; SigPatchConstOrPrimElements: 0
; SigInputVectors: 0
; SigOutputVectors[0]: 0
; SigOutputVectors[1]: 0
; SigOutputVectors[2]: 0
; SigOutputVectors[3]: 0
; EntryFunctionName: csMain
;
;
; Buffer Definitions:
;
; cbuffer 
; {
;
;   [64 x i8] (type annotation not present)
;
; }
;
; Resource bind info for 
; {
;
;   [1648 x i8] (type annotation not present)
; }
;
; Resource bind info for 
; {
;
;   [160 x i8] (type annotation not present)
; }
;
;
; Resource Bindings:
;
; Name                                 Type  Format         Dim      ID      HLSL Bind  Count
; ------------------------------ ---------- ------- ----------- ------- -------------- ------
;                                   cbuffer      NA          NA     CB0            cb0     1
;                                   texture  struct         r/o      T0             t4     1
;                                   texture  struct         r/o      T1            t11     1
;                                       UAV    byte         r/w      U0             u0     1
;
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

%dx.types.Handle = type { i8* }
%dx.types.CBufRet.f32 = type { float, float, float, float }
%dx.types.CBufRet.i32 = type { i32, i32, i32, i32 }
%dx.types.ResRet.f32 = type { float, float, float, float, i32 }
%dx.types.ResRet.i32 = type { i32, i32, i32, i32, i32 }
%"hostlayout.class.StructuredBuffer<EmitterProperties>" = type { %hostlayout.struct.EmitterProperties }
%hostlayout.struct.EmitterProperties = type { [4 x <4 x float>], [4 x <4 x float>], [4 x <4 x float>], float, float, float, float, float, float, float, float, <4 x float>, <4 x float>, i32, i32, i32, i32, [7 x i32], i32, [7 x i32], i32, i32, i32, i32, i32, i32, i32, i32, float, i32, i32, i32, float, float, float, float, float, i32, i32, i32, i32, i32, i32, i32, i32, [256 x float], [8 x <4 x float>], <4 x float>, <4 x float>, <4 x float>, [4 x i32] }
%"class.StructuredBuffer<EmitterGraphPerFrameData>" = type { %struct.EmitterGraphPerFrameData }
%struct.EmitterGraphPerFrameData = type { float, float, float, float, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, <4 x float>, i32, i32, i32, i32 }
%struct.RWByteAddressBuffer = type { i32 }
%GraphDefaultSimulateConstants = type { i32, i32, i32, i32, i32, i32, i32, i32, %struct.EmitterGraphSim }
%struct.EmitterGraphSim = type { i32, i32, i32, float, float, i32, i32, i32 }

define void @csMain() {
  %1 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %2 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 0, i32 1, i32 11, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %3 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 0, i32 0, i32 4, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %4 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 2, i32 0, i32 0, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %5 = call i32 @dx.op.threadId.i32(i32 93, i32 0)  ; ThreadId(component)
  %6 = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %4, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %7 = extractvalue %dx.types.CBufRet.f32 %6, 3
  %8 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %4, i32 2)  ; CBufferLoadLegacy(handle,regIndex)
  %9 = extractvalue %dx.types.CBufRet.i32 %8, 0
  %10 = extractvalue %dx.types.CBufRet.i32 %8, 1
  %11 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 0)  ; BufferLoad(srv,index,wot)
  %12 = extractvalue %dx.types.ResRet.f32 %11, 0
  %13 = extractvalue %dx.types.ResRet.f32 %11, 1
  %14 = extractvalue %dx.types.ResRet.f32 %11, 2
  %15 = extractvalue %dx.types.ResRet.f32 %11, 3
  %16 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 16)  ; BufferLoad(srv,index,wot)
  %17 = extractvalue %dx.types.ResRet.f32 %16, 0
  %18 = extractvalue %dx.types.ResRet.f32 %16, 1
  %19 = extractvalue %dx.types.ResRet.f32 %16, 2
  %20 = extractvalue %dx.types.ResRet.f32 %16, 3
  %21 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 32)  ; BufferLoad(srv,index,wot)
  %22 = extractvalue %dx.types.ResRet.f32 %21, 0
  %23 = extractvalue %dx.types.ResRet.f32 %21, 1
  %24 = extractvalue %dx.types.ResRet.f32 %21, 2
  %25 = extractvalue %dx.types.ResRet.f32 %21, 3
  %26 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 192)  ; BufferLoad(srv,index,wot)
  %27 = extractvalue %dx.types.ResRet.f32 %26, 0
  %28 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 220)  ; BufferLoad(srv,index,wot)
  %29 = extractvalue %dx.types.ResRet.f32 %28, 0
  %30 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %9, i32 260)  ; BufferLoad(srv,index,wot)
  %31 = extractvalue %dx.types.ResRet.i32 %30, 0
  %32 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %9, i32 368)  ; BufferLoad(srv,index,wot)
  %33 = extractvalue %dx.types.ResRet.i32 %32, 0
  %34 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %9, i32 372)  ; BufferLoad(srv,index,wot)
  %35 = extractvalue %dx.types.ResRet.i32 %34, 0
  %36 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %9, i32 376)  ; BufferLoad(srv,index,wot)
  %37 = extractvalue %dx.types.ResRet.i32 %36, 0
  %38 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 392)  ; BufferLoad(srv,index,wot)
  %39 = extractvalue %dx.types.ResRet.f32 %38, 0
  %40 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %9, i32 416)  ; BufferLoad(srv,index,wot)
  %41 = extractvalue %dx.types.ResRet.i32 %40, 0
  %42 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %9, i32 420)  ; BufferLoad(srv,index,wot)
  %43 = extractvalue %dx.types.ResRet.i32 %42, 0
  %44 = icmp ult i32 %5, %35
  br i1 %44, label %45, label %378, !dx.controlflow.hints !17

; <label>:45                                      ; preds = %0
  %46 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %9, i32 428)  ; BufferLoad(srv,index,wot)
  %47 = extractvalue %dx.types.ResRet.i32 %46, 0
  %48 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 204)  ; BufferLoad(srv,index,wot)
  %49 = extractvalue %dx.types.ResRet.f32 %48, 0
  %50 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %4, i32 3)  ; CBufferLoadLegacy(handle,regIndex)
  %51 = extractvalue %dx.types.CBufRet.i32 %50, 1
  %52 = icmp eq i32 %51, 1
  %53 = icmp ne i32 %10, 2147483647
  %54 = select i1 %53, i32 %10, i32 %9
  %55 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %54, i32 372)  ; BufferLoad(srv,index,wot)
  %56 = extractvalue %dx.types.ResRet.i32 %55, 0
  %57 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %54, i32 376)  ; BufferLoad(srv,index,wot)
  %58 = extractvalue %dx.types.ResRet.i32 %57, 0
  %59 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %54, i32 416)  ; BufferLoad(srv,index,wot)
  %60 = extractvalue %dx.types.ResRet.i32 %59, 0
  %61 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %54, i32 420)  ; BufferLoad(srv,index,wot)
  %62 = extractvalue %dx.types.ResRet.i32 %61, 0
  %63 = call float @dx.op.dot3.f32(i32 55, float %12, float %17, float %22, float %12, float %17, float %22)  ; Dot3(ax,ay,az,bx,by,bz)
  %64 = call float @dx.op.unary.f32(i32 25, float %63)  ; Rsqrt(value)
  %65 = fmul fast float 1.000000e+00, %12
  %66 = fmul fast float 1.000000e+00, %17
  %67 = fmul fast float 1.000000e+00, %22
  %68 = call float @dx.op.dot3.f32(i32 55, float %13, float %18, float %23, float %13, float %18, float %23)  ; Dot3(ax,ay,az,bx,by,bz)
  %69 = call float @dx.op.unary.f32(i32 25, float %68)  ; Rsqrt(value)
  %70 = fmul fast float 1.000000e+00, %13
  %71 = fmul fast float 1.000000e+00, %18
  %72 = fmul fast float 1.000000e+00, %23
  %73 = call float @dx.op.dot3.f32(i32 55, float %14, float %19, float %24, float %14, float %19, float %24)  ; Dot3(ax,ay,az,bx,by,bz)
  %74 = call float @dx.op.unary.f32(i32 25, float %73)  ; Rsqrt(value)
  %75 = fmul fast float 1.000000e+00, %14
  %76 = fmul fast float 1.000000e+00, %19
  %77 = fmul fast float 1.000000e+00, %24
  %78 = fcmp fast ogt float %29, 0.000000e+00
  %79 = select i1 %52, i32 %47, i32 0
  %80 = icmp ugt i32 %79, %56
  %81 = and i1 %52, %80
  br i1 %81, label %82, label %83, !dx.controlflow.hints !18

; <label>:82                                      ; preds = %45
  br label %83

; <label>:83                                      ; preds = %82, %45
  %84 = phi float [ 0.000000e+00, %82 ], [ %49, %45 ]
  %85 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %2, i32 0, i32 0)  ; BufferLoad(srv,index,wot)
  %86 = extractvalue %dx.types.ResRet.f32 %85, 0
  %87 = fmul fast float %86, %39
  %88 = add i32 %33, %5
  %89 = icmp ult i32 %88, %37
  br i1 %89, label %97, label %90, !dx.controlflow.hints !19

; <label>:90                                      ; preds = %83
  %91 = shl i32 %37, 1
  %92 = icmp ult i32 %88, %91
  br i1 %92, label %93, label %95, !dx.controlflow.hints !20

; <label>:93                                      ; preds = %90
  %94 = sub i32 %88, %37
  br label %97

; <label>:95                                      ; preds = %90
  %96 = urem i32 %88, %37
  br label %97

; <label>:97                                      ; preds = %95, %93, %83
  %98 = phi i32 [ %94, %93 ], [ %96, %95 ], [ %88, %83 ]
  %99 = mul i32 %98, %43
  %100 = add i32 %99, %41
  %101 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 %100, i32 undef)  ; BufferLoad(srv,index,wot)
  %102 = extractvalue %dx.types.ResRet.i32 %101, 3
  %103 = add i32 %100, 16
  %104 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 %103, i32 undef)  ; BufferLoad(srv,index,wot)
  %105 = extractvalue %dx.types.ResRet.i32 %104, 3
  %106 = add i32 %100, 32
  %107 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 %106, i32 undef)  ; BufferLoad(srv,index,wot)
  %108 = extractvalue %dx.types.ResRet.i32 %107, 3
  %109 = add i32 %100, 48
  %110 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 %109, i32 undef)  ; BufferLoad(srv,index,wot)
  %111 = extractvalue %dx.types.ResRet.i32 %110, 3
  %112 = bitcast i32 %111 to float
  %113 = add i32 %100, 84
  %114 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 %113, i32 undef)  ; BufferLoad(srv,index,wot)
  %115 = extractvalue %dx.types.ResRet.i32 %114, 0
  %116 = extractvalue %dx.types.ResRet.i32 %114, 1
  %117 = extractvalue %dx.types.ResRet.i32 %114, 2
  %118 = bitcast i32 %115 to float
  %119 = bitcast i32 %116 to float
  %120 = bitcast i32 %117 to float
  %121 = add i32 %100, 96
  %122 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 %121, i32 undef)  ; BufferLoad(srv,index,wot)
  %123 = extractvalue %dx.types.ResRet.i32 %122, 0
  %124 = extractvalue %dx.types.ResRet.i32 %122, 1
  %125 = extractvalue %dx.types.ResRet.i32 %122, 2
  %126 = bitcast i32 %123 to float
  %127 = bitcast i32 %124 to float
  %128 = bitcast i32 %125 to float
  %129 = fcmp fast ugt float %112, 0.000000e+00
  br i1 %129, label %130, label %378, !dx.controlflow.hints !21

; <label>:130                                     ; preds = %97
  %131 = icmp ult i32 %79, %58
  br i1 %131, label %139, label %132, !dx.controlflow.hints !19

; <label>:132                                     ; preds = %130
  %133 = shl i32 %58, 1
  %134 = icmp ult i32 %79, %133
  br i1 %134, label %135, label %137, !dx.controlflow.hints !20

; <label>:135                                     ; preds = %132
  %136 = sub i32 %79, %58
  br label %139

; <label>:137                                     ; preds = %132
  %138 = urem i32 %79, %58
  br label %139

; <label>:139                                     ; preds = %137, %135, %130
  %140 = phi i32 [ %136, %135 ], [ %138, %137 ], [ %79, %130 ]
  %141 = mul i32 %140, %62
  %142 = add i32 %141, %60
  %143 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %3, i32 %9, i32 1632)  ; BufferLoad(srv,index,wot)
  %144 = extractvalue %dx.types.ResRet.i32 %143, 0
  %145 = and i32 %144, 1023
  %146 = add i32 %142, %145
  %147 = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32 68, %dx.types.Handle %1, i32 %146, i32 undef)  ; BufferLoad(srv,index,wot)
  %148 = extractvalue %dx.types.ResRet.i32 %147, 3
  %149 = bitcast i32 %148 to float
  %150 = fcmp fast ole float %149, 0.000000e+00
  %151 = and i1 %52, %150
  br i1 %151, label %152, label %153, !dx.controlflow.hints !22

; <label>:152                                     ; preds = %139
  br label %153

; <label>:153                                     ; preds = %152, %139
  %154 = phi float [ 0x3E7AD7F2A0000000, %152 ], [ %84, %139 ]
  %155 = fsub fast float %154, %112
  %156 = fdiv fast float %155, %154
  %157 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 444)  ; BufferLoad(srv,index,wot)
  %158 = extractvalue %dx.types.ResRet.f32 %157, 0
  %159 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 448)  ; BufferLoad(srv,index,wot)
  %160 = extractvalue %dx.types.ResRet.f32 %159, 0
  %161 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 452)  ; BufferLoad(srv,index,wot)
  %162 = extractvalue %dx.types.ResRet.f32 %161, 0
  %163 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 456)  ; BufferLoad(srv,index,wot)
  %164 = extractvalue %dx.types.ResRet.f32 %163, 0
  %165 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 460)  ; BufferLoad(srv,index,wot)
  %166 = extractvalue %dx.types.ResRet.f32 %165, 0
  %167 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 464)  ; BufferLoad(srv,index,wot)
  %168 = extractvalue %dx.types.ResRet.f32 %167, 0
  %169 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 468)  ; BufferLoad(srv,index,wot)
  %170 = extractvalue %dx.types.ResRet.f32 %169, 0
  %171 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 472)  ; BufferLoad(srv,index,wot)
  %172 = extractvalue %dx.types.ResRet.f32 %171, 0
  %173 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 476)  ; BufferLoad(srv,index,wot)
  %174 = extractvalue %dx.types.ResRet.f32 %173, 0
  %175 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 480)  ; BufferLoad(srv,index,wot)
  %176 = extractvalue %dx.types.ResRet.f32 %175, 0
  %177 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 484)  ; BufferLoad(srv,index,wot)
  %178 = extractvalue %dx.types.ResRet.f32 %177, 0
  %179 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 488)  ; BufferLoad(srv,index,wot)
  %180 = extractvalue %dx.types.ResRet.f32 %179, 0
  %181 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 492)  ; BufferLoad(srv,index,wot)
  %182 = extractvalue %dx.types.ResRet.f32 %181, 0
  %183 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 496)  ; BufferLoad(srv,index,wot)
  %184 = extractvalue %dx.types.ResRet.f32 %183, 0
  %185 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 500)  ; BufferLoad(srv,index,wot)
  %186 = extractvalue %dx.types.ResRet.f32 %185, 0
  %187 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 504)  ; BufferLoad(srv,index,wot)
  %188 = extractvalue %dx.types.ResRet.f32 %187, 0
  %189 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 508)  ; BufferLoad(srv,index,wot)
  %190 = extractvalue %dx.types.ResRet.f32 %189, 0
  %191 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 512)  ; BufferLoad(srv,index,wot)
  %192 = extractvalue %dx.types.ResRet.f32 %191, 0
  %193 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 516)  ; BufferLoad(srv,index,wot)
  %194 = extractvalue %dx.types.ResRet.f32 %193, 0
  %195 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 520)  ; BufferLoad(srv,index,wot)
  %196 = extractvalue %dx.types.ResRet.f32 %195, 0
  %197 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 524)  ; BufferLoad(srv,index,wot)
  %198 = extractvalue %dx.types.ResRet.f32 %197, 0
  %199 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 528)  ; BufferLoad(srv,index,wot)
  %200 = extractvalue %dx.types.ResRet.f32 %199, 0
  %201 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 532)  ; BufferLoad(srv,index,wot)
  %202 = extractvalue %dx.types.ResRet.f32 %201, 0
  %203 = bitcast float %202 to i32
  %204 = icmp eq i32 %203, 0
  %205 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 536)  ; BufferLoad(srv,index,wot)
  %206 = extractvalue %dx.types.ResRet.f32 %205, 0
  %207 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 540)  ; BufferLoad(srv,index,wot)
  %208 = extractvalue %dx.types.ResRet.f32 %207, 0
  %209 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 544)  ; BufferLoad(srv,index,wot)
  %210 = extractvalue %dx.types.ResRet.f32 %209, 0
  %211 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 548)  ; BufferLoad(srv,index,wot)
  %212 = extractvalue %dx.types.ResRet.f32 %211, 0
  %213 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 552)  ; BufferLoad(srv,index,wot)
  %214 = extractvalue %dx.types.ResRet.f32 %213, 0
  %215 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 556)  ; BufferLoad(srv,index,wot)
  %216 = extractvalue %dx.types.ResRet.f32 %215, 0
  %217 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 560)  ; BufferLoad(srv,index,wot)
  %218 = extractvalue %dx.types.ResRet.f32 %217, 0
  %219 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 564)  ; BufferLoad(srv,index,wot)
  %220 = extractvalue %dx.types.ResRet.f32 %219, 0
  %221 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 568)  ; BufferLoad(srv,index,wot)
  %222 = extractvalue %dx.types.ResRet.f32 %221, 0
  %223 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 572)  ; BufferLoad(srv,index,wot)
  %224 = extractvalue %dx.types.ResRet.f32 %223, 0
  %225 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 576)  ; BufferLoad(srv,index,wot)
  %226 = extractvalue %dx.types.ResRet.f32 %225, 0
  %227 = call %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32 68, %dx.types.Handle %3, i32 %9, i32 580)  ; BufferLoad(srv,index,wot)
  %228 = extractvalue %dx.types.ResRet.f32 %227, 0
  %229 = fcmp fast ogt float %29, %154
  br i1 %229, label %231, label %230, !dx.controlflow.hints !23

; <label>:230                                     ; preds = %153
  br label %231

; <label>:231                                     ; preds = %230, %153
  %232 = phi float [ %112, %230 ], [ 0.000000e+00, %153 ]
  %233 = fsub fast float %232, %7
  %234 = call float @dx.op.unary.f32(i32 13, float %87)  ; Sin(value)
  %235 = fmul fast float %156, %156
  %236 = fmul fast float %235, %156
  %237 = call float @dx.op.dot4.f32(i32 56, float %214, float %216, float %218, float %220, float %236, float %235, float %156, float 1.000000e+00)  ; Dot4(ax,ay,az,aw,bx,by,bz,bw)
  %238 = call float @dx.op.binary.f32(i32 35, float %237, float 0.000000e+00)  ; FMax(a,b)
  %239 = call float @dx.op.binary.f32(i32 36, float %238, float 1.000000e+00)  ; FMin(a,b)
  %240 = call float @dx.op.dot4.f32(i32 56, float %206, float %208, float %210, float %212, float %236, float %235, float %156, float 1.000000e+00)  ; Dot4(ax,ay,az,aw,bx,by,bz,bw)
  %241 = call float @dx.op.binary.f32(i32 35, float %240, float 0.000000e+00)  ; FMax(a,b)
  %242 = call float @dx.op.binary.f32(i32 36, float %241, float 1.000000e+00)  ; FMin(a,b)
  %243 = call float @dx.op.dot4.f32(i32 56, float %222, float %224, float %226, float %228, float %236, float %235, float %156, float 1.000000e+00)  ; Dot4(ax,ay,az,aw,bx,by,bz,bw)
  %244 = call float @dx.op.binary.f32(i32 35, float %243, float 0.000000e+00)  ; FMax(a,b)
  %245 = call float @dx.op.binary.f32(i32 36, float %244, float 1.000000e+00)  ; FMin(a,b)
  %246 = fmul fast float %234, %7
  %247 = fmul fast float %127, %246
  %248 = fmul fast float %247, %242
  %249 = fsub fast float -0.000000e+00, %248
  %250 = call float @dx.op.unary.f32(i32 13, float %249)  ; Sin(value)
  %251 = call float @dx.op.unary.f32(i32 12, float %249)  ; Cos(value)
  %252 = fmul fast float %126, %246
  %253 = fmul fast float %252, %239
  %254 = fsub fast float -0.000000e+00, %253
  %255 = call float @dx.op.unary.f32(i32 13, float %254)  ; Sin(value)
  %256 = call float @dx.op.unary.f32(i32 12, float %254)  ; Cos(value)
  %257 = fsub fast float -0.000000e+00, %255
  %258 = call float @dx.op.tertiary.f32(i32 46, float %255, float %250, float 0.000000e+00)  ; FMad(a,b,c)
  %259 = call float @dx.op.tertiary.f32(i32 46, float %255, float %251, float 0.000000e+00)  ; FMad(a,b,c)
  %260 = call float @dx.op.tertiary.f32(i32 46, float %256, float %250, float 0.000000e+00)  ; FMad(a,b,c)
  %261 = call float @dx.op.tertiary.f32(i32 46, float %256, float %251, float 0.000000e+00)  ; FMad(a,b,c)
  %262 = fmul fast float %246, %128
  %263 = fmul fast float %262, %245
  %264 = call float @dx.op.unary.f32(i32 13, float %263)  ; Sin(value)
  %265 = call float @dx.op.unary.f32(i32 12, float %263)  ; Cos(value)
  %266 = fmul fast float %265, %251
  %267 = call float @dx.op.tertiary.f32(i32 46, float %264, float %258, float %266)  ; FMad(a,b,c)
  %268 = call float @dx.op.tertiary.f32(i32 46, float %264, float %256, float 0.000000e+00)  ; FMad(a,b,c)
  %269 = fmul fast float %250, %265
  %270 = fsub fast float -0.000000e+00, %269
  %271 = call float @dx.op.tertiary.f32(i32 46, float %264, float %259, float %270)  ; FMad(a,b,c)
  %272 = fmul fast float %251, %264
  %273 = fsub fast float -0.000000e+00, %272
  %274 = call float @dx.op.tertiary.f32(i32 46, float %265, float %258, float %273)  ; FMad(a,b,c)
  %275 = call float @dx.op.tertiary.f32(i32 46, float %265, float %256, float 0.000000e+00)  ; FMad(a,b,c)
  %276 = fmul fast float %264, %250
  %277 = call float @dx.op.tertiary.f32(i32 46, float %265, float %259, float %276)  ; FMad(a,b,c)
  %278 = fmul fast float %267, %65
  %279 = call float @dx.op.tertiary.f32(i32 46, float %66, float %274, float %278)  ; FMad(a,b,c)
  %280 = call float @dx.op.tertiary.f32(i32 46, float %67, float %260, float %279)  ; FMad(a,b,c)
  %281 = fmul fast float %268, %65
  %282 = call float @dx.op.tertiary.f32(i32 46, float %66, float %275, float %281)  ; FMad(a,b,c)
  %283 = call float @dx.op.tertiary.f32(i32 46, float %67, float %257, float %282)  ; FMad(a,b,c)
  %284 = fmul fast float %271, %65
  %285 = call float @dx.op.tertiary.f32(i32 46, float %66, float %277, float %284)  ; FMad(a,b,c)
  %286 = call float @dx.op.tertiary.f32(i32 46, float %67, float %261, float %285)  ; FMad(a,b,c)
  %287 = fmul fast float %267, %70
  %288 = call float @dx.op.tertiary.f32(i32 46, float %71, float %274, float %287)  ; FMad(a,b,c)
  %289 = call float @dx.op.tertiary.f32(i32 46, float %72, float %260, float %288)  ; FMad(a,b,c)
  %290 = fmul fast float %268, %70
  %291 = call float @dx.op.tertiary.f32(i32 46, float %71, float %275, float %290)  ; FMad(a,b,c)
  %292 = call float @dx.op.tertiary.f32(i32 46, float %72, float %257, float %291)  ; FMad(a,b,c)
  %293 = fmul fast float %271, %70
  %294 = call float @dx.op.tertiary.f32(i32 46, float %71, float %277, float %293)  ; FMad(a,b,c)
  %295 = call float @dx.op.tertiary.f32(i32 46, float %72, float %261, float %294)  ; FMad(a,b,c)
  %296 = fmul fast float %267, %75
  %297 = call float @dx.op.tertiary.f32(i32 46, float %76, float %274, float %296)  ; FMad(a,b,c)
  %298 = call float @dx.op.tertiary.f32(i32 46, float %77, float %260, float %297)  ; FMad(a,b,c)
  %299 = fmul fast float %268, %75
  %300 = call float @dx.op.tertiary.f32(i32 46, float %76, float %275, float %299)  ; FMad(a,b,c)
  %301 = call float @dx.op.tertiary.f32(i32 46, float %77, float %257, float %300)  ; FMad(a,b,c)
  %302 = fmul fast float %271, %75
  %303 = call float @dx.op.tertiary.f32(i32 46, float %76, float %277, float %302)  ; FMad(a,b,c)
  %304 = call float @dx.op.tertiary.f32(i32 46, float %77, float %261, float %303)  ; FMad(a,b,c)
  br i1 %204, label %309, label %305, !dx.controlflow.hints !24

; <label>:305                                     ; preds = %231
  %306 = call float @dx.op.dot4.f32(i32 56, float %194, float %196, float %198, float %200, float %236, float %235, float %156, float 1.000000e+00)  ; Dot4(ax,ay,az,aw,bx,by,bz,bw)
  %307 = call float @dx.op.binary.f32(i32 35, float %306, float 0x3EE4F8B580000000)  ; FMax(a,b)
  %308 = call float @dx.op.binary.f32(i32 36, float %307, float 1.000000e+00)  ; FMin(a,b)
  br label %319

; <label>:309                                     ; preds = %231
  %310 = call float @dx.op.dot4.f32(i32 56, float %170, float %172, float %174, float %176, float %236, float %235, float %156, float 1.000000e+00)  ; Dot4(ax,ay,az,aw,bx,by,bz,bw)
  %311 = call float @dx.op.binary.f32(i32 35, float %310, float 0x3EE4F8B580000000)  ; FMax(a,b)
  %312 = call float @dx.op.binary.f32(i32 36, float %311, float 1.000000e+00)  ; FMin(a,b)
  %313 = call float @dx.op.dot4.f32(i32 56, float %178, float %180, float %182, float %184, float %236, float %235, float %156, float 1.000000e+00)  ; Dot4(ax,ay,az,aw,bx,by,bz,bw)
  %314 = call float @dx.op.binary.f32(i32 35, float %313, float 0x3EE4F8B580000000)  ; FMax(a,b)
  %315 = call float @dx.op.binary.f32(i32 36, float %314, float 1.000000e+00)  ; FMin(a,b)
  %316 = call float @dx.op.dot4.f32(i32 56, float %186, float %188, float %190, float %192, float %236, float %235, float %156, float 1.000000e+00)  ; Dot4(ax,ay,az,aw,bx,by,bz,bw)
  %317 = call float @dx.op.binary.f32(i32 35, float %316, float 0x3EE4F8B580000000)  ; FMax(a,b)
  %318 = call float @dx.op.binary.f32(i32 36, float %317, float 1.000000e+00)  ; FMin(a,b)
  br label %319

; <label>:319                                     ; preds = %309, %305
  %320 = phi float [ %308, %305 ], [ %318, %309 ]
  %321 = phi float [ %308, %305 ], [ %312, %309 ]
  %322 = phi float [ %308, %305 ], [ %315, %309 ]
  %323 = fmul float %119, %322
  %324 = fmul float %118, %321
  %325 = fmul fast float %320, %120
  br i1 %78, label %326, label %331, !dx.controlflow.hints !25

; <label>:326                                     ; preds = %319
  %327 = fmul fast float %29, 2.000000e+00
  %328 = call float @dx.op.binary.f32(i32 35, float %327, float 0.000000e+00)  ; FMax(a,b)
  %329 = call float @dx.op.binary.f32(i32 36, float %328, float 1.000000e+00)  ; FMin(a,b)
  %330 = fsub fast float 1.000000e+00, %329
  br label %331

; <label>:331                                     ; preds = %326, %319
  %332 = phi float [ %330, %326 ], [ %27, %319 ]
  %333 = call float @dx.op.binary.f32(i32 35, float %332, float 0.000000e+00)  ; FMax(a,b)
  %334 = call float @dx.op.binary.f32(i32 36, float %333, float 1.000000e+00)  ; FMin(a,b)
  %335 = fsub fast float 1.000000e+00, %334
  %336 = fcmp fast ugt float %233, 0.000000e+00
  br i1 %336, label %340, label %337, !dx.controlflow.hints !26

; <label>:337                                     ; preds = %331
  switch i32 %31, label %340 [
    i32 1, label %338
    i32 2, label %339
  ], !dx.controlflow.hints !27

; <label>:338                                     ; preds = %337
  br label %340

; <label>:339                                     ; preds = %337
  br label %340

; <label>:340                                     ; preds = %339, %338, %337, %331
  %341 = phi float [ %233, %337 ], [ 0x3E7AD7F2A0000000, %339 ], [ %154, %338 ], [ %233, %331 ]
  %342 = fmul fast float %324, %280
  %343 = fmul fast float %324, %283
  %344 = fmul fast float %324, %286
  %345 = bitcast float %342 to i32
  %346 = bitcast float %343 to i32
  %347 = bitcast float %344 to i32
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %100, i32 undef, i32 %345, i32 %346, i32 %347, i32 %102, i8 15)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  %348 = fmul fast float %323, %289
  %349 = fmul fast float %323, %292
  %350 = fmul fast float %323, %295
  %351 = bitcast float %348 to i32
  %352 = bitcast float %349 to i32
  %353 = bitcast float %350 to i32
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %103, i32 undef, i32 %351, i32 %352, i32 %353, i32 %105, i8 15)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  %354 = fmul fast float %325, %298
  %355 = fmul fast float %325, %301
  %356 = fmul fast float %325, %304
  %357 = bitcast float %354 to i32
  %358 = bitcast float %355 to i32
  %359 = bitcast float %356 to i32
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %106, i32 undef, i32 %357, i32 %358, i32 %359, i32 %108, i8 15)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  %360 = bitcast float %15 to i32
  %361 = bitcast float %20 to i32
  %362 = bitcast float %25 to i32
  %363 = bitcast float %341 to i32
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %109, i32 undef, i32 %360, i32 %361, i32 %362, i32 %363, i8 15)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  %364 = call i32 @dx.op.legacyF32ToF16(i32 130, float %158)  ; LegacyF32ToF16(value)
  %365 = call i32 @dx.op.legacyF32ToF16(i32 130, float %160)  ; LegacyF32ToF16(value)
  %366 = shl i32 %365, 16
  %367 = or i32 %366, %364
  %368 = call i32 @dx.op.legacyF32ToF16(i32 130, float %162)  ; LegacyF32ToF16(value)
  %369 = add i32 %100, 64
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %369, i32 undef, i32 %367, i32 %368, i32 undef, i32 undef, i8 3)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  %370 = call i32 @dx.op.legacyF32ToF16(i32 130, float %164)  ; LegacyF32ToF16(value)
  %371 = call i32 @dx.op.legacyF32ToF16(i32 130, float %166)  ; LegacyF32ToF16(value)
  %372 = shl i32 %371, 16
  %373 = or i32 %372, %370
  %374 = call i32 @dx.op.legacyF32ToF16(i32 130, float %168)  ; LegacyF32ToF16(value)
  %375 = add i32 %100, 72
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %375, i32 undef, i32 %373, i32 %374, i32 undef, i32 undef, i8 3)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  %376 = bitcast float %335 to i32
  %377 = add i32 %100, 80
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %377, i32 undef, i32 %376, i32 undef, i32 undef, i32 undef, i8 1)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %113, i32 undef, i32 %115, i32 %116, i32 %117, i32 undef, i8 7)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  call void @dx.op.bufferStore.i32(i32 69, %dx.types.Handle %1, i32 %121, i32 undef, i32 %123, i32 %124, i32 %125, i32 undef, i8 7)  ; BufferStore(uav,coord0,coord1,value0,value1,value2,value3,mask)
  br label %378

; <label>:378                                     ; preds = %340, %97, %0
  ret void
}

; Function Attrs: nounwind readnone
declare i32 @dx.op.threadId.i32(i32, i32) #0

; Function Attrs: nounwind readnone
declare i32 @dx.op.legacyF32ToF16(i32, float) #0

; Function Attrs: nounwind readnone
declare float @dx.op.binary.f32(i32, float, float) #0

; Function Attrs: nounwind readnone
declare float @dx.op.dot3.f32(i32, float, float, float, float, float, float) #0

; Function Attrs: nounwind readnone
declare float @dx.op.unary.f32(i32, float) #0

; Function Attrs: nounwind readnone
declare float @dx.op.dot4.f32(i32, float, float, float, float, float, float, float, float) #0

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32, %dx.types.Handle, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32, %dx.types.Handle, i32) #1

; Function Attrs: nounwind readnone
declare float @dx.op.tertiary.f32(i32, float, float, float) #0

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandle(i32, i8, i32, i32, i1) #1

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(i32, %dx.types.Handle, i32, i32) #1

; Function Attrs: nounwind readonly
declare %dx.types.ResRet.f32 @dx.op.bufferLoad.f32(i32, %dx.types.Handle, i32, i32) #1

; Function Attrs: nounwind
declare void @dx.op.bufferStore.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i8) #2

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind readonly }
attributes #2 = { nounwind }

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!2}
!dx.shaderModel = !{!3}
!dx.resources = !{!4}
!dx.entryPoints = !{!14}

!0 = !{!"dxcoob 1.8.2505.0"}
!1 = !{i32 1, i32 0}
!2 = !{i32 1, i32 10}
!3 = !{!"cs", i32 6, i32 0}
!4 = !{!5, !10, !12, null}
!5 = !{!6, !8}
!6 = !{i32 0, %"hostlayout.class.StructuredBuffer<EmitterProperties>"* undef, !"", i32 0, i32 4, i32 1, i32 12, i32 0, !7}
!7 = !{i32 1, i32 1648}
!8 = !{i32 1, %"class.StructuredBuffer<EmitterGraphPerFrameData>"* undef, !"", i32 0, i32 11, i32 1, i32 12, i32 0, !9}
!9 = !{i32 1, i32 160}
!10 = !{!11}
!11 = !{i32 0, %struct.RWByteAddressBuffer* undef, !"", i32 0, i32 0, i32 1, i32 11, i1 false, i1 false, i1 false, null}
!12 = !{!13}
!13 = !{i32 0, %GraphDefaultSimulateConstants* undef, !"", i32 0, i32 0, i32 1, i32 64, null}
!14 = !{void ()* @csMain, !"csMain", null, !4, !15}
!15 = !{i32 0, i64 16, i32 4, !16}
!16 = !{i32 64, i32 1, i32 1}
!17 = distinct !{!17, !"dx.controlflow.hints", i32 1}
!18 = distinct !{!18, !"dx.controlflow.hints", i32 1}
!19 = distinct !{!19, !"dx.controlflow.hints", i32 1}
!20 = distinct !{!20, !"dx.controlflow.hints", i32 1}
!21 = distinct !{!21, !"dx.controlflow.hints", i32 1}
!22 = distinct !{!22, !"dx.controlflow.hints", i32 1}
!23 = distinct !{!23, !"dx.controlflow.hints", i32 1}
!24 = distinct !{!24, !"dx.controlflow.hints", i32 1}
!25 = distinct !{!25, !"dx.controlflow.hints", i32 1}
!26 = distinct !{!26, !"dx.controlflow.hints", i32 1}
!27 = distinct !{!27, !"dx.controlflow.hints", i32 1}

