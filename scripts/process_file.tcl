# This proc will take in a file, the list of uncovered lines and the
# the corresponding uncovered lines will be highlighted in the window

# SPECIAL NOTE : In order to avoid multiple re-read of the same file, it
# is much better to read in a file at the very first time when the
# corresponding tree-node is selected. The file contents are kept as 
# part of the global filecontent hashtable.

set fileContent(0)        0
set file_name             0
set start_line            0
set end_line              0
set line_summary_total    0
set line_summary_hit      0
set toggle_summary_total  0
set toggle_summary_hit01  0
set toggle_summary_hit10  0
set curr_mod_name         0

# TODO : 
# Suppose that a really large verilog file has a lot of lines uncovered.
# Obviously, if we use a list and lsearch then the time it will take to
# print the total listing will be a bit too-much. As of now, we are 
# using lsearch, but will certainly optimize later.

proc process_module_line_cov {} {

  global fileContent file_name start_line end_line
  global curr_mod_name

  tcl_func_get_filename $curr_mod_name

  if {[catch {set fileText $fileContent($file_name)}]} {
    if {[catch {set fp [open $file_name "r"]}]} {
      tk_messageBox -message "File $file_name Not Found!" \
                    -title "No File" -icon error
      return
    } 
    set fileText [read $fp]
    set fileContent($file_name) $fileText
    close $fp
  }

  # Get start and end line numbers of this module
  set start_line 0
  set end_line   0
  tcl_func_get_module_start_and_end $curr_mod_name

  # Get line summary information and display this now
  tcl_func_get_line_summary $curr_mod_name

  calc_and_display_line_cov

}

proc calc_and_display_line_cov {} {

  global cov_type uncov_type mod_inst_type mod_list
  global uncovered_lines covered_lines
  global curr_mod_name

  if {$curr_mod_name != 0} {

    # Get list of uncovered/covered lines
    if {$uncov_type == 1} {
      set uncovered_lines 0
      tcl_func_collect_uncovered_lines $curr_mod_name
    }
    if {$cov_type == 1} {
      set covered_lines 0
      tcl_func_collect_covered_lines $curr_mod_name
    }

    display_line_cov

  }

}

proc display_line_cov {} {

  global fileContent file_name
  global uncov_fgColor uncov_bgColor cov_fgColor cov_bgColor
  global uncovered_lines covered_lines uncov_type cov_type
  global start_line end_line
  global line_summary_total line_summary_hit

  # Populate information bar
  .bot.info configure -text "Filename: $file_name"

  .bot.txt tag configure uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
  .bot.txt tag configure cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor

  # Allow us to write to the text box
  .bot.txt configure -state normal

  # Clear the text-box before any insertion is being made
  .bot.txt delete 1.0 end

  set contents [split $fileContent($file_name) \n]
  set linecount 1

  if {$end_line != 0} {

    # First, populate the summary information
    .covbox.ht configure -text "$line_summary_hit"
    .covbox.tt configure -text "$line_summary_total"

    # Next, populate text box with file contents including highlights for covered/uncovered lines
    foreach phrase $contents {
      if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
        set line [format {%7d  %s} $linecount [append phrase "\n"]]
        if {[expr $uncov_type == 1] && [expr [lsearch $uncovered_lines $linecount] != -1]} {
          .bot.txt insert end $line uncov_colorMap
        } elseif {[expr $cov_type == 1] && [expr [lsearch $covered_lines $linecount] != -1]} {
          .bot.txt insert end $line cov_colorMap
        } else {
          .bot.txt insert end $line
        }
      }
      incr linecount
    }

  }

  # Now cause the text box to be read-only again
  .bot.txt configure -state disabled

  return

}

proc process_module_toggle_cov {} {

  global fileContent file_name start_line end_line
  global curr_mod_name

  if {$curr_mod_name != 0} {

    tcl_func_get_filename $curr_mod_name

    if {[catch {set fileText $fileContent($file_name)}]} {
      if {[catch {set fp [open $file_name "r"]}]} {
        tk_messageBox -message "File $file_name Not Found!" \
                      -title "No File" -icon error
        return
      } 
      set fileText [read $fp]
      set fileContent($file_name) $fileText
      close $fp
    }

    # Get start and end line numbers of this module
    set start_line 0
    set end_line   0
    tcl_func_get_module_start_and_end $curr_mod_name

    # Get line summary information and display this now
    tcl_func_get_toggle_summary $curr_mod_name

    calc_and_display_toggle_cov

  }

}

proc calc_and_display_toggle_cov {} {

  global cov_type uncov_type mod_inst_type mod_list
  global uncovered_toggles covered_toggles
  global curr_mod_name

  if {$curr_mod_name != 0} {

    # Get list of uncovered/covered lines
    if {$uncov_type == 1} {
      set uncovered_toggles 0
      tcl_func_collect_uncovered_toggles $curr_mod_name
    }
    if {$cov_type == 1} {
      set covered_toggles 0
      tcl_func_collect_covered_toggles $curr_mod_name
    }

    display_toggle_cov

  }

}

proc display_toggle_cov {} {

  global fileContent file_name
  global uncov_fgColor uncov_bgColor cov_fgColor cov_bgColor
  global uncovered_toggles covered_toggles
  global uncov_type cov_type
  global start_line end_line
  global toggle_summary_total toggle_summary_hit01 toggle_summary_hit10
  global cov_rb mod_inst_type mod_list
  global toggle01_verbose toggle10_verbose toggle_width
  global curr_mod_name

  if {$curr_mod_name != 0} {

    # Populate information bar
    .bot.info configure -text "Filename: $file_name"

    .bot.txt tag configure uncov_colorMap -foreground $uncov_fgColor -background $uncov_bgColor
    .bot.txt tag configure cov_colorMap   -foreground $cov_fgColor   -background $cov_bgColor

    # Allow us to write to the text box
    .bot.txt configure -state normal

    # Clear the text-box before any insertion is being made
    .bot.txt delete 1.0 end

    set contents [split $fileContent($file_name) \n]
    set linecount 1

    if {$end_line != 0} {

      # First, populate the summary information
      .covbox.ht configure -text "$toggle_summary_hit01"
      .covbox.tt configure -text "$toggle_summary_total"

      # Next, populate text box with file contents including highlights for covered/uncovered lines
      foreach phrase $contents {
        if [expr [expr $start_line <= $linecount] && [expr $end_line >= $linecount]] {
          set line [format {%7d  %s} $linecount [append phrase "\n"]]
          .bot.txt insert end $line
        }
        incr linecount
      }

      # Finally, set toggle information
      if {[expr $uncov_type == 1] && [expr [llength $uncovered_toggles] > 0]} {
        set cmd_enter  ".bot.txt tag add uncov_enter"
        set cmd_button ".bot.txt tag add uncov_button"
        set cmd_leave  ".bot.txt tag add uncov_leave"
        foreach entry $uncovered_toggles {
          set cmd_enter  [concat $cmd_enter  $entry]
          set cmd_button [concat $cmd_button $entry]
          set cmd_leave  [concat $cmd_leave  $entry]
        }
        eval $cmd_enter
        eval $cmd_button
        eval $cmd_leave
        .bot.txt tag configure uncov_button -underline true -foreground $uncov_fgColor -background $uncov_bgColor
        .bot.txt tag bind uncov_enter <Enter> {
          set curr_cursor [.bot.txt cget -cursor]
          .bot.txt configure -cursor hand2
        }
        .bot.txt tag bind uncov_leave <Leave> {
          .bot.txt configure -cursor $curr_cursor
        }
        .bot.txt tag bind uncov_button <ButtonPress-1> {
          create_toggle_window $curr_mod_name [.bot.txt get {current wordstart} {current wordend}]
        }
      } 

      if {[expr $cov_type == 1] && [expr [llength $covered_toggles] > 0]} {
        set cmd_cov ".bot.txt tag add cov_highlight"
        foreach entry $covered_toggles {
          set cmd_cov [concat $cmd_cov $entry]
        }
        eval $cmd_cov
        .bot.txt tag configure cov_highlight -foreground $cov_fgColor -background $cov_bgColor
      }

    }

  }

  # Now cause the text box to be read-only again
  .bot.txt configure -state disabled

  return

}
 
proc process_module_comb_cov {} {

  global start_line end_line
  global 

  display_comb_cov

} 

proc display_comb_cov {} {
 
  global fgColor bgColor

  # Configure text area
  .bot.txt tag configure colorMap -foreground $fgColor -background $bgColor

  # Clear the text-box before any insertion is being made
  .bot.txt delete 1.0 end

}

proc process_module_fsm_cov {} {

  global start_line end_line

  display_fsm_cov

}

proc display_fsm_cov {} {

  global fgColor bgColor

  # Configure text area
  .bot.txt tag configure colorMap -foreground $fgColor -background $bgColor

  # Clear the text-box before any insertion is being made
  .bot.txt delete 1.0 end

}
