set comb_ul_ip         0
set comb_curr_uline_id 0

proc K {x y} {
  set x
}

proc comb_calc_indexes {uline line first} {

  global comb_uline_indexes comb_ul_ip

  set i 0

  if {[expr $first == 0] && [expr $comb_ul_ip == 1]} {
    set i          [string first "-" $uline 0]
    set comb_ul_ip 0
  } else {
    set i [string first "|" $uline 0]
  }

  while {$i != -1} {
    if {$first == 1} {
      set first              0
      set comb_uline_indexes "$line.$i"
      set comb_ul_ip         1
    } else {
      if {$comb_ul_ip == 1} {
        lappend comb_uline_indexes "$line.[expr $i + 1]"
        set comb_ul_ip 0
      } else {
        lappend comb_uline_indexes "$line.$i"
        set comb_ul_ip 1
      }
    }
    set i [string first "|" $uline [expr $i + 1]]
  }

  if {[expr $i == -1] && [expr $comb_ul_ip == 1]} {
    lappend comb_uline_indexes "$line.end"
  }
    
}

proc display_comb_info {} {

  global comb_code comb_uline_groups comb_ulines
  global comb_uline_indexes comb_first_uline
  global comb_gen_ulines comb_curr_uline_id
  global comb_curr_cursor
  global uncov_fgColor

  set curr_uline 0

  # Remove any tags associated with the toggle text box
  .combwin.f.t tag delete comb_enter
  .combwin.f.t tag delete comb_leave
  .combwin.f.t tag delete comb_bp1
  .combwin.f.t tag delete comb_bp2

  # Allow us to clear out text box and repopulate
  .combwin.f.t configure -state normal

  # Clear the text-box before any insertion is being made
  .combwin.f.t delete 1.0 end

  # Get length of comb_code list
  set code_len [llength $comb_code]

  # Generate underlines
  generate_underlines

  # Write code and underlines
  set curr_line 1
  set first     1
  for {set j 0} {$j<$code_len} {incr j} {
    .combwin.f.t insert end "[lindex $comb_code $j]\n"
    incr curr_line
    .combwin.f.t insert end "[lindex $comb_gen_ulines $j]\n"
    comb_calc_indexes [lindex $comb_gen_ulines $j] $curr_line $first
    incr curr_line
    set first 0
    .combwin.f.t insert end "\n"
    incr curr_line
  }

  # Keep user from writing in text boxes
  .combwin.f.t configure -state disabled

  # Add expression tags and bindings
  if {[llength $comb_uline_indexes] > 0} {
    eval ".combwin.f.t tag add comb_enter $comb_uline_indexes"
    eval ".combwin.f.t tag add comb_leave $comb_uline_indexes"
    eval ".combwin.f.t tag add comb_bp1 $comb_uline_indexes"
    eval ".combwin.f.t tag add comb_bp2 $comb_uline_indexes"
    .combwin.f.t tag configure comb_bp1 -foreground $uncov_fgColor
    .combwin.f.t tag bind comb_enter <Enter> {
      set comb_curr_cursor [.combwin.f.t cget -cursor]
      .combwin.f.t configure -cursor hand2
    }
    .combwin.f.t tag bind comb_leave <Leave> {
      .combwin.f.t configure -cursor $comb_curr_cursor
    }
    .combwin.f.t tag bind comb_bp1 <ButtonPress-1> {
      .combwin.f.t configure -cursor $comb_curr_cursor
      set selected_range [.combwin.f.t tag prevrange comb_bp1 {current + 1 chars}]
      set redraw [move_display_down [get_expr_index_from_range $selected_range]]
      if {$redraw == 1} {
        set text_x [.combwin.f.t xview]
        set text_y [.combwin.f.t yview]
        display_comb_info
        .combwin.f.t xview moveto [lindex $text_x 0]
        .combwin.f.t yview moveto [lindex $text_y 0]
      } 
    }
    .combwin.f.t tag bind comb_bp2 <ButtonPress-2> {
      .combwin.f.t configure -cursor $comb_curr_cursor
      set selected_range [.combwin.f.t tag prevrange comb_bp2 {current + 1 chars}]
      set redraw [move_display_up [get_expr_index_from_range $selected_range]]
      if {$redraw == 1} {
        set text_x [.combwin.f.t xview]
        set text_y [.combwin.f.t yview]
        display_comb_info
        .combwin.f.t xview moveto [lindex $text_x 0]
        .combwin.f.t yview moveto [lindex $text_y 0]
      } 
    }
  }

}

proc display_comb_coverage {ulid} {

  global curr_mod_name comb_expr_cov

  # Allow us to clear out text box and repopulate
  .combwin.f.tc configure -state normal

  # Clear the text-box before any insertion is being made
  .combwin.f.tc delete 1.0 end

  # Get combinational coverage information
  set comb_expr_cov ""
  tcl_func_get_comb_coverage $curr_mod_name $ulid

  # Display coverage information
  .combwin.f.tc insert end "\n\n"
  foreach line $comb_expr_cov {
    .combwin.f.tc insert end "$line\n"
  }

  # Keep user from writing in text boxes
  .combwin.f.tc configure -state disabled

}

proc get_expr_index_from_range {selected_range} {

  global comb_uline_exprs comb_curr_uline_id
  global curr_mod_name

  # Get range information
  set start_info [split [lindex $selected_range 0] .]

  set i     0
  set found 0

  while {[expr $i < [llength $comb_uline_exprs]] && [expr $found == 0]} {
  
    set curr_expr [lindex $comb_uline_exprs $i]

    # If the current expression is being displayed
    if {[lindex $curr_expr 0] == 1} {

      set j         0
      set lines     [lindex $curr_expr 3]
      set line_num  [llength $lines]
      set curr_line [lindex $lines 0]
   
      while {[expr $j < $line_num] && [expr $found == 0]} {
        if {[lindex $curr_line 3] == [expr [lindex $start_info 0] / 3]} {
          if {[lindex $curr_line 1] == [lindex $start_info 1]} {
            set found 1
          }
        }
        incr j
        set curr_line [lindex $lines $j]
      }
  
    }

    if {$found == 0} {
      incr i
    }

  }

  if {$found == 1} {

    # Get current expression underline ID for lookup purposes
    set comb_curr_uline_id [lindex $curr_expr 2]

    # Output current expression in information bar
    if {$comb_curr_uline_id != 0} {

      # Display information in textbox
      display_comb_coverage $comb_curr_uline_id

      # Place information into infobar
      .combwin.f.info configure -text "Current Expression: $comb_curr_uline_id"

    }

    return $i

  } else {
    return -1
  }

}

proc get_underline_expressions {parent} {

  global comb_uline_exprs comb_uline_groups
  global comb_ulines

  # Get information from parent expression
  set parent_expr  [lindex $comb_uline_exprs $parent]
  set parent_level [lindex $parent_expr 1]
  set parent_ulid  [lindex $parent_expr 2]

  # Calculate the current level by adding one to the parent expression
  set child_level [expr $parent_level + 1]

  # Iterate through list of lines that parent consumes getting children
  set parent_lines    [lindex $parent_expr 3]
  set parent_line_num [llength $parent_lines]
  set uline_ip        0
  set child_ids       [list]

  for {set i 0} {$i < $parent_line_num} {incr i} {

    # Get current line
    set line [lindex $parent_lines $i]

    # Get current parent line information
    set index      [lindex $line 0]
    set start_char [lindex $line 1]
    set end_char   [lindex $line 2]
    set code_index [lindex $line 3]

    if {$child_level < [lindex $comb_uline_groups $code_index]} {

      # Extract children from this line
      set curr_uline [lindex $comb_ulines [expr $index - 1]]
      set curr_char  0

      if {$uline_ip == 1} {
        set curr_char [string first "-" $curr_uline $curr_char] 
        set uline_ip  0
      } else {
        set curr_char [string first "|" $curr_uline $curr_char] 
      }

      while {[expr $curr_char != -1] && [expr $curr_char <= $end_char]} {
        if {$uline_ip == 0} {
          set uline_ip 1
          set start_char $curr_char
          set child_ulid [regexp -inline -start $curr_char -- {\d+} $curr_uline]
        } else {
          set uline_ip 0
          set child_line [list [expr $index - 1] $start_char [expr $curr_char + 1] $code_index]
          lappend child_lines $child_line

          # This child is done so add it to the expression list now
          lappend comb_uline_exprs [list 0 $child_level $child_ulid $child_lines $parent]
          set child_lines [list]
          lappend child_ids [expr [llength $comb_uline_exprs] - 1]

        }
        set curr_char [string first "|" $curr_uline [expr $curr_char + 1]]
        # set child_ulid [regexp -inline -start [expr $curr_char + 1] -- {\d+} $curr_uline]
      }

      if {$uline_ip == 1} { 
        set child_line [list [expr $index - 1] $start_char [string length $curr_uline] $code_index]
        lappend child_lines $child_line
      }

    }
    
  }

  # Set child IDs to this parent
  lappend parent_expr $child_ids
  set comb_uline_exprs [lreplace [K $comb_uline_exprs [set comb_uline_exprs ""]] $parent $parent $parent_expr]

}

proc move_display_down {parent_index} {

  global comb_uline_exprs

  set redraw 0

  # Get children list from parent expression
  set parent_expr [lindex $comb_uline_exprs $parent_index]
  set children    [lindex $parent_expr 5]

  # Iterate through all children, setting the display variable to 1
  foreach child $children {
    set child_expr       [lindex $comb_uline_exprs $child]
    set child_expr       [lreplace [K $child_expr [set child_expr ""]] 0 0 1]
    set comb_uline_exprs [lreplace [K $comb_uline_exprs [set comb_uline_exprs ""]] $child $child $child_expr]
  }

  if {[llength $children] > 0} {

    # Set parent display variable to 0
    set parent_expr      [lreplace [K $parent_expr [set parent_expr ""]] 0 0 0]
    set comb_uline_exprs [lreplace [K $comb_uline_exprs [set comb_uline_exprs ""]] $parent_index $parent_index $parent_expr]
    set redraw 1

  }

  return $redraw

}

proc zero_display_children {parent_index} {

  global comb_uline_exprs

  set parent_expr [lindex $comb_uline_exprs $parent_index]

  foreach child [lindex $parent_expr 5] {

    # Clear child
    set child_expr       [lindex $comb_uline_exprs $child]
    set child_expr       [lreplace [K $child_expr [set child_expr ""]] 0 0 0]
    set comb_uline_exprs [lreplace [K $comb_uline_exprs [set comb_uline_exprs ""]] $child $child $child_expr]

    # Clear grandchild
    zero_display_children $child
    
  }

}

proc move_display_up {child_index} {

  global comb_uline_exprs

  set redraw 0

  # Get parent expression from child
  set child_expr [lindex $comb_uline_exprs $child_index]
  set parent     [lindex $child_expr 4]

  if {$parent != 0} {

    # Set parent display to 1
    set parent_expr      [lindex $comb_uline_exprs $parent]
    set parent_expr      [lreplace [K $parent_expr [set parent_expr ""]] 0 0 1]
    set comb_uline_exprs [lreplace [K $comb_uline_exprs [set comb_uline_exprs ""]] $parent $parent $parent_expr]

    # Zero out display value for all children expressions
    zero_display_children $parent

    set redraw 1

  }

  return $redraw

}
 
# Create a list containing locations of underlined expressions in the comb_ulines list
# Each entry in the list will be organized as follows:
#   displayed level uline_id {{index start_char end_char code_linenum}*} parent_id {children_ids...}
# parent_id == -1 means that this doesn't have a parent
# id is based on index in list
proc organize_underlines {} {

  global comb_ulines comb_uline_groups comb_uline_exprs

  set code_len  [llength $comb_uline_groups]
  set curr_line 0

  for {set i 0} {$i < $code_len} {incr i} {
    set index [expr [lindex $comb_uline_groups $i] + $curr_line]
    lappend line_info [list $index 0 [string length [lindex $comb_ulines [expr $index - 1]]] $i]
    set curr_line [expr $curr_line + [lindex $comb_uline_groups $i]]
  }

  lappend comb_uline_exprs [list 0 -1 -1 $line_info -1]

  # Get all child expressions
  set i 0
  while {$i < [llength $comb_uline_exprs]} {
    get_underline_expressions $i
    incr i
  }

  # Set the display value in the children of the top-level
  move_display_down 0

}

proc generate_underlines {} {

  global comb_uline_exprs comb_ulines comb_code
  global comb_gen_ulines

  # Clear comb_gen_ulines
  set comb_gen_ulines ""

  # Create blank slate for underlines
  foreach code_line $comb_code {
    lappend comb_gen_ulines [string repeat " " [string length $code_line]]
  }

  set expr_num [llength $comb_uline_exprs]

  foreach curr_expr $comb_uline_exprs {

    # If this expression is to be displayed, generate the output now
    if {[lindex $curr_expr 0] == 1} {

      set lines [lindex $curr_expr 3]

      foreach line $lines {

        # Extract information from current expression line
        set uline_index [lindex $line 0]
        set uline_start [lindex $line 1]
        set uline_end   [lindex $line 2]
        set code_index  [lindex $line 3]

        # Replace spaces with underline information
        set uline [lindex $comb_gen_ulines $code_index]
        set uline [string replace $uline $uline_start $uline_end [string range [lindex $comb_ulines $uline_index] $uline_start $uline_end]]
        set comb_gen_ulines [lreplace [K $comb_gen_ulines [set comb_gen_ulines ""]] $code_index $code_index $uline]

      }

    }

  }

}

proc create_comb_window {mod_name expr_id} {

  global comb_code comb_uline_groups comb_ulines

  # Now create the window and set the grab to this window
  if {[winfo exists .combwin] == 0} {

    # Create new window
    toplevel .combwin
    wm title .combwin "Combinational Logic Coverage - Verbose"

    frame .combwin.f

    # Add expression information
    label .combwin.f.l0 -anchor w -text "Expression:"
    text  .combwin.f.t -height 20 -width 100 -xscrollcommand ".combwin.f.thb set" -yscrollcommand ".combwin.f.tvb set" -wrap none
    scrollbar .combwin.f.thb -orient horizontal -command ".combwin.f.t xview"
    scrollbar .combwin.f.tvb -orient vertical   -command ".combwin.f.t yview"

    # Add expression coverage information
    label .combwin.f.l1 -anchor w -text "Coverage Information:  ('*' represents a case that was not hit)"
    text  .combwin.f.tc -height 20 -width 100 -xscrollcommand ".combwin.f.chb set" -yscrollcommand ".combwin.f.cvb set" -wrap none -state disabled
    scrollbar .combwin.f.chb -orient horizontal -command ".combwin.f.tc xview"
    scrollbar .combwin.f.cvb -orient vertical   -command ".combwin.f.tc yview"

    # Create bottom information bar
    label .combwin.f.info -anchor w

    # Pack the widgets into the bottom frame
    grid rowconfigure    .combwin.f 1 -weight 1
    grid columnconfigure .combwin.f 0 -weight 1
    grid columnconfigure .combwin.f 1 -weight 0
    grid .combwin.f.l0   -row 0 -column 0 -sticky nsew
    grid .combwin.f.t    -row 1 -column 0 -sticky nsew
    grid .combwin.f.thb  -row 2 -column 0 -sticky ew
    grid .combwin.f.tvb  -row 1 -column 1 -sticky ns
    grid .combwin.f.l1   -row 3 -column 0 -sticky nsew
    grid .combwin.f.tc   -row 4 -column 0 -sticky nsew
    grid .combwin.f.chb  -row 5 -column 0 -sticky ew
    grid .combwin.f.cvb  -row 4 -column 1 -sticky ns
    grid .combwin.f.info -row 6 -column 0 -sticky ew

    pack .combwin.f -fill both -expand yes -side bottom

  }

  # Get verbose combinational logic information
  set comb_code         "" 
  set comb_uline_groups "" 
  set comb_ulines       ""
  tcl_func_get_comb_expression $mod_name $expr_id

  organize_underlines

  # Write combinational logic with level 0 underline information in text box
  display_comb_info

}
