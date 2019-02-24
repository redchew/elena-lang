..\..\bin\elc accumulator\accumulator.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

..\..\bin\elc ackermann\ackerman.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

..\..\bin\elc addfield\addfield.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

..\..\bin\elc anonymrec\anonymrec.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

..\..\bin\elc aplusb\aplusb.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

..\..\bin\elc applycallback\arraycallback.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

..\..\bin\elc arithmeticint\arithmeticint.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem ..\..\bin\elc arithmmean\arithmmean.prj
..\..\bin\elc arrayconcat\arrayconcat.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem ..\..\bin\elc arraymode\arraymode.prj

..\..\bin\elc arrays\arrays.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem ..\..\bin\elc associativearrays\associativearrays.prj
rem ..\..\bin\elc bestshuffle\bestshuffle.prj
rem ..\..\bin\elc binary\binary.prj
..\..\bin\elc amb\amb.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem ..\..\bin\elc bitwise\bitwise.prj
rem ..\..\bin\elc brackets\brackets.prj
..\..\bin\elc anagram\anagram.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem ..\..\bin\elc median\median.prj
rem ..\..\bin\elc smavg\smavg.prj
rem ..\..\bin\elc caesar\caesar.prj
rem ..\..\bin\elc charmatch\charmatch.prj

..\..\bin\elc arithmeval\arithmeval.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem ..\..\bin\elc combinations\combinations.prj
rem ..\..\bin\elc bullscows\bullscows.prj
rem ..\..\bin\elc calendar\calendar.prj
..\..\bin\elc doors\doors.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

..\..\bin\elc twentyfour\twentyfour.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

..\..\bin\elc simple_windowed_app\simple_windowed_app.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem ..\..\bin\elc gui_component_interaction\gui_component_interaction.prj
rem ..\..\bin\elc string_append\string_append.prj
rem ..\..\bin\elc string_case\string_case.prj
rem ..\..\bin\elc string_comparison\string_comparision.prj
rem ..\..\bin\elc string_concatenation\string_concatenation.prj
rem ..\..\bin\elc string_interpolation\string_interpolation.prj
rem ..\..\bin\elc string_matching\string_matching.prj
rem ..\..\bin\elc string_prepend\string_prepend.prj

..\..\bin\elc ninetynine\ninetynine.prj
@echo off 
if %ERRORLEVEL% EQU -2 GOTO CompilerError
@echo on

rem ..\..\bin\elc evolutionary\evolutionary.prj
rem ..\..\bin\elc compare_str_list\complist.prj
rem ..\..\bin\elc dynamic_var\dynamic_var.prj
rem ..\..\bin\elc firstclass\firstclass.prj
rem ..\..\bin\elc loop_multiple_arrays\loopma.prj
rem ..\..\bin\elc knutalg\knutalg.prj
rem ..\..\bin\elc manboy\manboy.prj
rem ..\..\bin\elc reverse_words_in_string\rev_words.prj
rem ..\..\bin\elc tokenizer\tokenizer.prj
rem ..\..\bin\elc trigonometric\trigonometric.prj
rem ..\..\bin\elc toppergroup\toppergroup.prj
rem ..\..\bin\elc twelvestats\twelvestats.prj
rem ..\..\bin\elc truncprime\truncprime.prj
rem ..\..\bin\elc treeview\treeview.prj
rem ..\..\bin\elc gameoflife\gameoflife.prj
rem ..\..\bin\elc zhangsuen\zhangsuen.prj
rem ..\..\bin\elc zeckendorf\zeckendorf.prj
rem ..\..\bin\elc zeckendorf_arithm\zeckendorf_arithm.prj 
rem ..\..\bin\elc ycombinator\ycombinator.prj

@echo off 
goto:eof
@echo on

:CompilerError
echo ELC returns error %ERRORLEVEL%
@echo off
goto:eof
@echo on
