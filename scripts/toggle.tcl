set sig_name        ""
set curr_toggle_ptr ""

proc display_toggle {curr_index} {

  global prev_index next_index curr_toggle_ptr

  # Get range of current signal
  set curr_range [.bot.right.txt tag prevrange uncov_button "$curr_index + 1 chars"]

  # Calculate the current signal string
  set curr_signal [string trim [lindex [split [.bot.right.txt get [lindex $curr_range 0] [lindex $curr_range 1]] "\["] 0]]

  # Make sure that the selected signal is visible in the text box and is shown as selected
  set_pointer curr_toggle_ptr [lindex [split [lindex $curr_range 0] .] 0]

  # Get range of previous signal
  set prev_index [lindex [.bot.right.txt tag prevrange uncov_button [lindex $curr_index 0]] 0]

  # Get range of next signal
  set next_index [lindex [.bot.right.txt tag nextrange uncov_button [lindex $curr_range 1]] 0]

  # Now create the toggle window
  create_toggle_window $curr_signal

}

proc set_toggle_pointer {line} {

  global curr_toggle_ptr

  # Allow the textbox to be changed
  .bot.right.txt configure -state normal

  # Delete old cursor, it if is displayed
  if {$curr_toggle_ptr != ""} {
    .bot.right.txt delete $curr_toggle_ptr.0 $curr_toggle_ptr.3
    .bot.right.txt insert $curr_toggle_ptr.0 "   "
  }

  # Display new pointer
  .bot.right.txt delete $line.0 $line.3
  .bot.right.txt insert $line.0 "-->"

  # Set the textbox to not be changed
  .bot.right.txt configure -state disabled

  # Make sure that we can see the current toggle pointer in the textbox
  .bot.right.txt see $line.0

  # Update curr_toggle_ptr
  set curr_toggle_ptr $line

}

proc create_toggle_window {signal} {

  global toggle01_verbose toggle10_verbose toggle_msb toggle_lsb
  global sig_name prev_index next_index
  global curr_funit_name curr_funit_type

  set sig_name $signal

  # Now create the window and set the grab to this window
  if {[winfo exists .togwin] == 0} {

    # Create new window
    toplevel .togwin
    wm title .togwin "Toggle Coverage - Verbose"

    frame .togwin.f -relief raised -borderwidth 1

    # Add toggle information
    label .togwin.f.l_01 -anchor w -text "Toggle 0->1"
    label .togwin.f.l_10 -anchor w -text "Toggle 1->0"
    text  .togwin.f.t -height 2 -width 40 -xscrollcommand ".togwin.f.hb set" -wrap none -spacing1 2 -spacing3 3
    scrollbar .togwin.f.hb -orient horizontal -command ".togwin.f.t xview"

    # Create bottom information bar
    label .togwin.f.info -anchor e

    # Create bottom button bar
    frame .togwin.bf -relief raised -borderwidth 1
    button .togwin.bf.close -text "Close" -width 10 -command {
      destroy .togwin
    }
    button .togwin.bf.help -text "Help" -width 10 -command {
      help_show_manual toggle
    }
    button .togwin.bf.prev -text "<--" -command {
      display_toggle $prev_index
    }
    button .togwin.bf.next -text "-->" -command {
      display_toggle $next_index
    }

    # Pack the buttons into the button frame
    pack .togwin.bf.prev  -side left  -padx 8 -pady 4
    pack .togwin.bf.next  -side left  -padx 8 -pady 4
    pack .togwin.bf.help  -side right -padx 8 -pady 4
    pack .togwin.bf.close -side right -padx 8 -pady 4

    # Pack the widgets into the bottom frame
    grid rowconfigure    .togwin.f 1 -weight 1
    grid columnconfigure .togwin.f 0 -weight 1
    grid columnconfigure .togwin.f 1 -weight 0
    grid .togwin.f.t    -row 0 -rowspan 2 -column 0 -sticky nsew
    grid .togwin.f.l_01 -row 0 -column 1 -sticky nsew
    grid .togwin.f.l_10 -row 1 -column 1 -sticky nsew
    grid .togwin.f.hb   -row 2 -column 0 -sticky ew
    grid .togwin.f.info -row 3 -column 0 -sticky ew

    pack .togwin.f  -fill both
    pack .togwin.bf -fill x

  }

  # Disable next or previous buttons if valid
  if {$prev_index == ""} {
    .togwin.bf.prev configure -state disabled
  } else {
    .togwin.bf.prev configure -state normal
  }
  if {$next_index == ""} {
    .togwin.bf.next configure -state disabled
  } else {
    .togwin.bf.next configure -state normal
  }

  # Get verbose toggle information
  set toggle01_verbose 0
  set toggle10_verbose 0
  set toggle_msb       0
  set toggle_lsb       0
  tcl_func_get_toggle_coverage $curr_funit_name $curr_funit_type $signal

  # Allow us to clear out text box
  .togwin.f.t configure -state normal

  # Remove any tags associated with the toggle text box
  .togwin.f.t tag delete tog_motion

  # Clear the text-box before any insertion is being made
  .togwin.f.t delete 1.0 end

  # Write toggle 0->1 and 1->0 information in text box
  .togwin.f.t insert end "$toggle01_verbose\n$toggle10_verbose" 

  # Keep user from writing in text boxes
  .togwin.f.t configure -state disabled

  # Create leave bindings for textboxes
  bind .togwin.f.t <Leave> {
    .togwin.f.info configure -text "$sig_name\[$toggle_msb:$toggle_lsb\]"
  }

  # Add toggle tags and bindings
  .togwin.f.t tag add  tog_motion 1.0 {end - 1 chars}
  .togwin.f.t tag bind tog_motion <Motion> {
    set tmp [lindex [split [.togwin.f.t index current] "."] 1]
    set tmp [expr $toggle_msb - $tmp]
    .togwin.f.info configure -text "$sig_name\[$tmp\]"
  }

  # Right justify the toggle information
  if {[expr [expr $toggle_msb - $toggle_lsb] + 1] <= 32} { 
    .togwin.f.t tag configure tog_motion -justify right
  } else {
    .togwin.f.t tag configure tog_motion -justify left
    .togwin.f.t xview moveto 1.0
  }

  .togwin.f.info configure -text "$sig_name\[$toggle_msb:$toggle_lsb\]"

  # Raise this window to the foreground
  raise .togwin

}
