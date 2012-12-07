#! /usr/opt/bin/fth -vdDs
\ examples/scripts/xm.fth.  Generated from xm.fth.in by configure.
\ 
\ xm.fth -- example from libxm.html 
\ (see ftp://ccrma-ftp.stanford.edu/pub/Lisp/libxm.tar.gz)
\
\ usage: ./xm.fth -display :0.0

dl-load libxm Init_libxm

"Test" *argc* *argv* array->list FapplicationShellWidgetClass
#( FXmNallowShellResize #t ) #() FXtVaOpenApplication value shell-app
shell-app 1 array-ref value app
shell-app 0 array-ref value shell
shell FXtDisplay FDefaultScreenOfDisplay FBlackPixelOfScreen value black
shell #( FXmNtitle "Hi!" ) FXtSetValues drop

"main-pane" FxmFormWidgetClass shell
#( FXmNforeground       black
   FXmNtopAttachment    FXmATTACH_FORM
   FXmNbottomAttachment FXmATTACH_FORM
   FXmNleftAttachment   FXmATTACH_FORM
   FXmNrightAttachment  FXmATTACH_FORM
   FXmNallowResize      #t ) FXtVaCreateManagedWidget value main-pane

"Xen-Forth test widget [show values]"
FxmPushButtonWidgetClass main-pane #() 0 FXtCreateManagedWidget ( wid )
FXmNactivateCallback
lambda: <{ w c i -- }>
  "\\  widget: %p\n" #( w ) fth-print
  "\\ context: %p\n" #( c ) fth-print
  "\\    info: %p\n" #( i ) fth-print
  "\\  reason: %s\n" #( i Freason ) fth-print
; black FXtAddCallback drop

shell FXtRealizeWidget drop
app FXtAppMainLoop drop

\ xm.fth ends here
