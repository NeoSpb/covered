set dump_filetypes {
  {{VCD Dumpfiles} {.vcd}}
  {{LXT Dumpfiles} {.lxt}}
  {{LXT2 Dumpfiles} {.lxt2}}
  {{All Files}     * }
}

proc create_new_cdd {} {

  global cddgen_sel cddgen_fname cddgen_auto_open cddgen_sname
  global cdd_filename

  if {[winfo exists .newwin] == 0} {

    toplevel .newwin
    wm title .newwin "Create New CDD"
    wm transient .newwin .

    # Create labelframe that will hold the contents
    panedwindow .newwin.p 
    
    # Add panes
    .newwin.p add [create_new_cdd_source .newwin.p.source] -width 750 -height 450
    .newwin.p add [create_new_cdd_name   .newwin.p.name]   -width 750 -height 450 -hide true
    .newwin.p add [create_new_cdd_dump   .newwin.p.dump]   -width 750 -height 450 -hide true
    .newwin.p add [create_new_cdd_parse  .newwin.p.parse1] -width 750 -height 450 -hide true
    .newwin.p add [create_new_cdd_parse2 .newwin.p.parse2] -width 750 -height 450 -hide true
    .newwin.p add [create_new_cdd_output .newwin.p.output] -width 750 -height 450 -hide true

    # Wait for objects to be created
    update idletasks

    # Initialize global variables
    set cddgen_sel       "options"
    set cddgen_fname     ""
    set cddgen_auto_open 0
    set cddgen_sname     ""
    set cdd_filename     ""
  
    # Pack the panedwindow
    pack .newwin.p -fill both -expand yes
 
  }

  # Finally, raise this window
  raise .newwin

}

proc parse_vpi_ts {str rnum rscale} {

  upvar $rnum   tnum
  upvar $rscale tscale

  set unknown 0

  if {[string range $str end-1 end] eq "1s"} {
    set ttnum   [string range $str 0 end-1]
    set ttscale "s"
  } elseif {[string range $str end-1 end] eq "0s"} {
    set ttnum   [string range $str 0 end-1]
    set ttscale "s"
  } elseif {[string range $str end-1 end] eq "ms"} {
    set ttnum   [string range $str 0 end-2]
    set ttscale "ms"
  } elseif {[string range $str end-1 end] eq "us"} {
    set ttnum   [string range $str 0 end-2]
    set ttscale "us"
  } elseif {[string range $str end-1 end] eq "ns"} {
    set ttnum   [string range $str 0 end-2]
    set ttscale "ns"
  } elseif {[string range $str end-1 end] eq "ps"} {
    set ttnum   [string range $str 0 end-2]
    set ttscale "ps"
  } elseif {[string range $str end-1 end] eq "fs"} {
    set ttnum   [string range $str 0 end-2]
    set ttscale "fs"
  } else {
    set unknown 1
  }

  if {$unknown == 0} {
    if {$ttnum eq "1" || $ttnum eq "10" || $ttnum eq "100"} {
      set tnum   $ttnum
      set tscale $ttscale
    }
  }

}

proc create_arglist_for_new_cdd {} {
  
  global cdd_filename
  global dump_vpi_none dump_file
  global vpi_file vpi_ts vpi_ts_num1 vpi_ts_scale1 vpi_ts_num2 vpi_ts_scale2
  global simulator
  global toplevel_name inst_name delay_type
  global race_cond_action race_cond_pragma race_cond_pragma_name
  global assert_ovl design_generation
  global exclude_always exclude_assign exclude_initial exclude_final exclude_pragma exclude_pragma_name

  set args {}

  if {$cdd_filename ne ""} {
    lappend args "-o $cdd_filename"
  }

  if {$dump_vpi_none eq "dump"} {
    set ext [file extension $dump_file]
    if {$ext eq ".vcd"} {
      lappend args "-vcd $dump_file"
    } elseif {$ext eq ".lxt" || $ext eq ".lxt2"} {
      lappend args "-lxt $dump_file"
    }
  } elseif {$dump_vpi_none eq "vpi"} {
    if {$vpi_file eq ""} {
      lappend args "-vpi"
    } else {
      lappend args "-vpi $vpi_file"
    }
    if {$vpi_ts == 1} {
      lappend args "-vpi_ts $vpi_ts_num1$vpi_ts_scale1/$vpi_ts_num2$vpi_ts_scale2"
    }
  }

  if {$toplevel_name ne ""} {
    lappend args "-t $toplevel_name"
  }

  if {$inst_name ne ""} {
    lappend args "-i $inst_name"
  }

  if {$delay_type eq "Min"} {
    lappend args "-T min"
  } elseif {$delay_type eq "Typ"} {
    lappend args "-T typ"
  } elseif {$delay_type eq "Max"} {
    lappend args "-T max"
  }

  if {$race_cond_action eq "Silent"} {
    lappend args "-rS"
  } elseif {$race_cond_action eq "Warning"} {
    lappend args "-rW"
  } elseif {$race_cond_action eq "Error"} {
    lappend args "-rE"
  } elseif {$race_cond_action eq "Ignore"} {
    lappend args "-rI"
  }
  if {$race_cond_pragma == 1} {
    if {$race_cond_pragma_name eq "racecheck"} {
      lappend args "-rP"
    } else {
      lappend args "-rP $race_cond_pragma_name"
    }
  }

  if {$assert_ovl == 1} {
    lappend args "-A ovl"
  }

  if {$design_generation eq "Verilog 1995"} {
    lappend args "-g 1"
  } elseif {$design_generation eq "Verilog 2001"} {
    lappend args "-g 2"
  } elseif {$design_generation eq "System Verilog"} {
    lappend args "-g 3"
  }

  # exclude_always exclude_assign exclude_initial exclude_final exclude_pragma exclude_pragma_name
  if {$exclude_always == 1} {
    lappend args "-ea"
  }
  if {$exclude_assign == 1} {
    lappend args "-ec"
  }
  if {$exclude_initial == 1} {
    lappend args "-ei"
  }
  if {$exclude_final == 1} {
    lappend args "-ef"
  }
  if {$exclude_pragma == 1} {
    if {$exclude_pragma_name eq "coverage"} {
      lappend args "-ep"
    } else {
      lappend args "-ep $exclude_pragma_name"
    }
  }

  foreach row [.newwin.p.parse2.opts.lbf.lb get 0 end] {
    if {[lindex $row 0] eq "Source File"} {
      lappend args "-v [lindex $row 1]"
    } elseif {[lindex $row 0] eq "Library Directory"} {
      lappend args "-y [lindex $row 1]"
    } elseif {[lindex $row 0] eq "Library Extension"} {
      lappend args "+libext[lindex $row 1]"
    } elseif {[lindex $row 0] eq "Include Directory"} {
      lappend args "-I [lindex $row 1]"
    } elseif {[lindex $row 0] eq "Define"} {
      lappend args "-D [lindex $row 1]"
    } elseif {[lindex $row 0] eq "Param Override"} {
      lappend args "-P [lindex $row 1]"
    } elseif {[lindex $row 0] eq "FSM"} {
      lappend args "-F [lindex $row 1]"
    } elseif {[lindex $row 0] eq "Module Exclusion"} {
      lappend args "-e [lindex $row 1]"
    } elseif {[lindex $row 0] eq "Module Generation"} {
      lappend args "-g [lindex $row 1]"
    } elseif {[lindex $row 0] eq "Command File"} {
      lappend args "-f [lindex $row 1]"
    }
  }

  return $args

}

proc generate_new_cdd {w} {

  global COVERED dump_vpi_none

  # Create the argument list
  set cmdline [join [create_arglist_for_new_cdd]]

  # Advance to the output window
  goto_next_pane $w

  # Open and clear the output contained in the given textbox
  .newwin.p.output.f.t configure -state normal
  .newwin.p.output.f.t delete 1.0 end

  # Generate the specified CDD file, grabbing the output so that we can display it
  set fp [open "|$COVERED score $cmdline" "r+"]

  while {[gets $fp line] >= 0} {
    if {[string range $line 0 23] eq "Dynamic memory allocated" && $dump_vpi_none ne "none"} {
      .newwin.p.output.fo.cb configure -state normal
    }
    .newwin.p.output.f.t insert end "$line\n"
  }

  close $fp

  # Disable the output textbox state
  .newwin.p.output.f.t configure -state disabled

}

proc read_score_option_file {w fname} {

  global cdd_filename cddgen_fname
  global dump_vpi_none dump_file
  global vpi_file vpi_ts vpi_ts_num1 vpi_ts_scale1 vpi_ts_num2 vpi_ts_scale2
  global toplevel_name inst_name delay_type
  global race_cond_action race_cond_pragma race_cond_pragma_name
  global assert_ovl design_generation
  global exclude_always exclude_assign exclude_initial exclude_final exclude_pragma exclude_pragma_name

  if {[catch {set fp [open $cddgen_fname "r"]}]} {
    tk_messageBox -message "File $cddgen_fname Not Found!" -title "No File" -icon error
  }

  set contents [join [list [read $fp]]]

  for {set i 0} {$i<[llength $contents]} {incr i} {

    if {[lindex $contents $i] eq "-o" || [lindex $contents $i] eq "-cdd"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set cdd_filename [lindex $contents $i]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-vcd"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set dump_vpi_none "dump"
        set dump_file     [lindex $contents $i]
        handle_new_cdd_dump_states .newwin.p.dump
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-lxt"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set dump_vpi_none "dump"
        set dump_file     [lindex $contents $i]
        handle_new_cdd_dump_states .newwin.p.dump
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-vpi"} {
      set dump_vpi_none "vpi"
      handle_new_cdd_dump_states .newwin.p.dump
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set vpi_file [lindex $contents $i]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-vpi_ts"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set vpi_ts 1
        set vpi_ts_elems [split [lindex $contents $i] /]
        parse_vpi_ts [lindex $vpi_ts_elems 0] vpi_ts_num1 vpi_ts_scale1
        parse_vpi_ts [lindex $vpi_ts_elems 1] vpi_ts_num2 vpi_ts_scale2
        handle_new_cdd_dump_vpi_timescale .newwin.p.dump
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-t"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set toplevel_name [lindex $contents $i]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-i"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set inst_name [lindex $contents $i]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-T"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[lindex $contents $i] eq "min"} {
          set delay_type "Min"
        } elseif {[lindex $contents $i] eq "typ"} {
          set delay_type "Typ"
        } elseif {[lindex $contents $i] eq "max"} {
          set delay_type "Max"
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-rI"} {
      set race_cond_action "Ignore"

    } elseif {[lindex $contents $i] eq "-rS"} {
      set race_cond_action "Silent"

    } elseif {[lindex $contents $i] eq "-rW"} {
      set race_cond_action "Warning"

    } elseif {[lindex $contents $i] eq "-rE"} {
      set race_cond_action "Error"

    } elseif {[lindex $contents $i] eq "-rP"} {
      set race_cond_pragma 1
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set race_cond_pragma_name [lindex $contents $i]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-A"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[lindex $contents $i] eq "ovl"} {
          set assert_ovl 1
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-g"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[string first "=" [lindex $contents $i]] != -1} {
          .newwin.p.parse2.opts.lbf.lb insert end [list "Module Generation" [lindex $contents $i]]
        } else {
          if {[lindex $contents $i] eq "1"} {
            set design_generation "Verilog 1995"
          } elseif {[lindex $contents $i] eq "2"} {
            set design_generation "Verilog 2001"
          } elseif {[lindex $contents $i] eq "3"} {
            set design_generation "System Verilog"
          }
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-ec"} {
      set exclude_assign 1

    } elseif {[lindex $contents $i] eq "-ea"} {
      set exclude_always 1

    } elseif {[lindex $contents $i] eq "-ei"} {
      set exclude_initial 1

    } elseif {[lindex $contents $i] eq "-ef"} {
      set exclude_final 1

    } elseif {[lindex $contents $i] eq "-ep"} {
      set exclude_pragma 1
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        set exclude_pragma_name [lindex $contents $i]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-v"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[file isfile [lindex $contents $i]] == 1} {
          .newwin.p.parse2.opts.lbf.lb insert end [list "Source File" [lindex $contents $i]]
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-y"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[file isdirectory [lindex $contents $i]] == 1} {
          .newwin.p.parse2.opts.lbf.lb insert end [list "Library Directory" [lindex $contents $i]]
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[string range [lindex $contents $i] 0 7] eq "+libext+"} {
      .newwin.p.parse2.opts.lbf.lb insert end [list "Library Extension" [string range [lindex $contents $i] 7 end]]

    } elseif {[lindex $contents $i] eq "-I"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        if {[file isdirectory [lindex $contents $i]] == 1} {
          .newwin.p.parse2.opts.lbf.lb insert end [list "Include Directory" [lindex $contents $i]]
        }
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-D"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        .newwin.p.parse2.opts.lbf.lb insert end [list "Define" [lindex $contents $i]]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-P"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        .newwin.p.parse2.opts.lbf.lb insert end [list "Param Override" [lindex $contents $i]]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-F"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        .newwin.p.parse2.opts.lbf.lb insert end [list "FSM" [lindex $contents $i]] 
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-e"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        .newwin.p.parse2.opts.lbf.lb insert end [list "Module Exclusion" [lindex $contents $i]]
      } else {
        set i [expr $i - 1]
      }

    } elseif {[lindex $contents $i] eq "-f"} {
      incr i
      if {[string index [lindex $contents $i] 0] ne "-"} {
        .newwin.p.parse2.opts.lbf.lb insert end [list "Command File" [lindex $contents $i]]
      } else {
        set i [expr $i - 1]
      }

    }

  }

  close $fp

}

proc setup_cdd_generate_options {w} {

  global cddgen_fname
  global cdd_filename
  global dump_vpi_none dump_file
  global vpi_file vpi_ts vpi_ts_num1 vpi_ts_scale1 vpi_ts_num2 vpi_ts_scale2
  global simulator
  global toplevel_name inst_name delay_type
  global race_cond_action race_cond_pragma race_cond_pragma_name
  global assert_ovl design_generation
  global exclude_always exclude_assign exclude_initial exclude_final exclude_pragma exclude_pragma_name

  if {$cddgen_fname ne ""} {

    # Perform global variable initialization here
    set cdd_filename          ""
    set dump_vpi_none         "none"
    set dump_file             ""
    set vpi_file              ""
    set vpi_ts                "0"
    set vpi_ts_num1           "1"
    set vpi_ts_scale1         "s"
    set vpi_ts_num2           "1"
    set vpi_ts_scale2         "s"
    set simulator             "Icarus Verilog"
    set toplevel_name         ""
    set inst_name             ""
    set delay_type            "None"
    set race_cond_action      "None"
    set race_cond_pragma      0
    set race_cond_pragma_name "racecheck"
    set assert_ovl            0
    set design_generation     "System Verilog"
    set exclude_always        0
    set exclude_assign        0
    set exclude_initial       0
    set exclude_final         0
    set exclude_pragma        0
    set exclude_pragma_name   "coverage"

    read_score_option_file $w $cddgen_fname

  } else {

    # Otherwise, set global variables to desired default values
    set cdd_filename         ""
    set dump_vpi_none         "none"
    set dump_file             ""
    set vpi_file              ""
    set vpi_ts                "0"
    set vpi_ts_num1           "1"
    set vpi_ts_scale1         "s"
    set vpi_ts_num2           "1"
    set vpi_ts_scale2         "s"
    set simulator             "Icarus Verilog"
    set toplevel_name         ""
    set inst_name             ""
    set delay_type            "None"
    set race_cond_action      "None"
    set race_cond_pragma      0
    set race_cond_pragma_name "racecheck"
    set assert_ovl            0
    set design_generation     "System Verilog"
    set exclude_always        0
    set exclude_assign        0
    set exclude_initial       0
    set exclude_final         0
    set exclude_pragma        0
    set exclude_pragma_name   "coverage"

  }

  # If -vcd, -lxt or -vpi was not specified, make sure that we set the "Parse Design Only" bits accordingly
  if {$dump_vpi_none eq "none"} {
    handle_new_cdd_dump_states .newwin.p.dump
  }

}

proc handle_new_cdd_source {w {fname ""} {use_fname 0}} {

  global cddgen_sel cddgen_fname

  if {$use_fname == 0} {
    set fname $cddgen_fname
  }

  if {$cddgen_sel eq "options"} {
    $w.f.fc.e configure -state disabled
    $w.f.fc.b configure -state disabled
    $w.bf.next configure -state normal
  } else {
    $w.f.fc.e configure -state normal
    $w.f.fc.b configure -state normal
    if {$fname eq "" || [file isfile $fname] == 0} {
      $w.bf.next configure -state disabled
    } else {
      $w.bf.next configure -state normal
    }
  }

  return 1

}

proc create_new_cdd_source {w} {

  global cddgen_sel cddgen_fname

  # Create the upper widget frame for this pane
  frame $w

  # Create upper widgets
  frame $w.f
  frame $w.f.fu
  frame $w.f.fc
  frame $w.f.fl
  radiobutton $w.f.fc.rb_opts -anchor w -text "Create CDD by interactively selecting options" -variable cddgen_sel -value "options" \
     -command "handle_new_cdd_source $w"
  radiobutton $w.f.fc.rb_file -anchor w -text "Create CDD by using option file" -variable cddgen_sel -value "file" \
     -command "handle_new_cdd_source $w"
  entry  $w.f.fc.e -state disabled -textvariable cddgen_fname -validate all -vcmd "handle_new_cdd_source $w %P 1"
  button $w.f.fc.b -text "Browse..." -state disabled -command {
    set fname [tk_getOpenFile -title "Select a Score Command Option File" -parent .newwin]
    if {$fname ne ""} {
      set cddgen_fname $fname
    }
  }
  grid columnconfigure $w.f.fc 1 -weight 1
  grid $w.f.fc.rb_opts -row 0 -column 0 -columnspan 3 -sticky news -pady 10
  grid $w.f.fc.rb_file -row 1 -column 0 -sticky news -pady 10
  grid $w.f.fc.e       -row 1 -column 1 -sticky news -pady 10
  grid $w.f.fc.b       -row 1 -column 2 -sticky news -pady 10

  pack $w.f.fu -fill both -expand 1
  pack $w.f.fc -fill x
  pack $w.f.fl -fill both -expand 1

  # Create button frame
  frame $w.bf
  button $w.bf.help   -width 10 -text "Help" -command {
    puts "Help!"
  }
  button $w.bf.cancel -width 10 -text "Cancel" -command "destroy [winfo toplevel $w]"
  button $w.bf.next   -width 10 -text "Next" -command "
    setup_cdd_generate_options $w
    goto_next_pane $w
  "
  pack $w.bf.help   -side right -padx 4 -pady 4
  pack $w.bf.cancel -side right -padx 4 -pady 4
  pack $w.bf.next   -side right -padx 4 -pady 4

  # Pack top-level frames
  pack $w.f  -fill both -expand yes
  pack $w.bf -fill x

  return $w

}

proc create_new_cdd_name_browse {w} {

  global cdd_filename file_types

  set tmp_cdd [tk_getSaveFile -title {Select CDD Filename} -filetypes $file_types -parent [winfo parent $w]]

  # If a filename was selected, set the cdd_filename and generate an event to the entry widget
  if {$tmp_cdd ne ""} {
    set cdd_filename $tmp_cdd
    event generate $w.cdd.e <Return>
  }

}

proc create_new_cdd_name_update_next {w} {

  global cdd_filename

  if {$cdd_filename ne ""} {
    $w.bf.next configure -state normal
  } else {
    $w.bf.next configure -state disabled
  }

  return 1

}

proc create_new_cdd_name {w} {

  global cdd_filename

  # Create top-level frame
  frame  $w

  # Create top frames
  frame  $w.fu
  frame  $w.fl
  frame  $w.cdd
  label  $w.cdd.l -text "CDD name:"
  entry  $w.cdd.e -textvariable cdd_filename -validate all
  button $w.cdd.b -text "Browse" -width 10 -command "create_new_cdd_name_browse $w"
    
  pack $w.cdd.l -side left  -padx 3 -pady 3 -fill y
  pack $w.cdd.e -side left  -padx 3 -pady 3 -fill x -expand 1
  pack $w.cdd.b -side right -padx 3 -pady 3 -fill y

  # Create button bar
  frame  $w.bf
  button $w.bf.help   -width 10 -text "Help" -command {
    puts "TBD"
  }
  button $w.bf.cancel -width 10 -text "Cancel" -command "destroy [winfo toplevel $w]"
  button $w.bf.next   -width 10 -text "Next"   -command "goto_next_pane $w" -state disabled
  button $w.bf.prev   -width 10 -text "Back"   -command "goto_prev_pane $w"
  pack $w.bf.help   -side right -padx 4 -pady 4
  pack $w.bf.cancel -side right -padx 4 -pady 4
  pack $w.bf.next   -side right -padx 4 -pady 4
  pack $w.bf.prev   -side left  -padx 4 -pady 4

  $w.cdd.e configure -vcmd "create_new_cdd_name_update_next $w"

  # Pack frames
  pack $w.fu  -fill both -expand 1
  pack $w.cdd -fill x
  pack $w.fl  -fill both -expand 1
  pack $w.bf  -fill x

  return $w

}

proc handle_new_cdd_dump_states {w} {

  global dump_vpi_none vpi_ts dump_file

  if {$dump_vpi_none eq "none"} {

    $w.dump.ed configure -state disabled
    $w.dump.bd configure -state disabled
    $w.dump.ev configure -state disabled
    $w.dump.mv configure -state disabled
    $w.bf.next configure -state normal
    set_widget_state $w.dump.ts disabled

  } elseif {$dump_vpi_none eq "dump"} {

    $w.dump.ed configure -state normal
    $w.dump.bd configure -state normal
    $w.dump.ev configure -state disabled
    $w.dump.mv configure -state disabled
    if {$dump_file ne ""} {
      $w.bf.next configure -state normal
    } else {
      $w.bf.next configure -state disabled
    }
    set_widget_state $w.dump.ts disabled

  } elseif {$dump_vpi_none eq "vpi"} { 

    $w.dump.ed    configure -state disabled
    $w.dump.bd    configure -state disabled
    $w.dump.ev    configure -state normal
    $w.dump.mv    configure -state normal
    $w.dump.ts.cb configure -state normal
    $w.bf.next    configure -state normal
    if {$vpi_ts == 1} {
      set_widget_state $w.dump.ts normal
    }

  }

}

proc handle_new_cdd_dump_vpi_timescale {w} {

  global vpi_ts

  if {$vpi_ts == 0} {

    $w.dump.ts.n1 configure -state disabled
    $w.dump.ts.s1 configure -state disabled
    $w.dump.ts.l  configure -state disabled
    $w.dump.ts.n2 configure -state disabled
    $w.dump.ts.s2 configure -state disabled

  } else {

    $w.dump.ts.n1 configure -state normal
    $w.dump.ts.s1 configure -state normal
    $w.dump.ts.l  configure -state normal
    $w.dump.ts.n2 configure -state normal
    $w.dump.ts.s2 configure -state normal

  }

}

proc create_new_cdd_dump {w} {

  global dump_filetypes
  global dump_vpi_none dump_file
  global vpi_file vpi_ts vpi_ts_num1 vpi_ts_scale1 vpi_ts_num2 vpi_ts_scale2
  global simulator

  # Add dumpfile widgets
  frame         $w
  frame         $w.fu
  frame         $w.fl
  frame         $w.dump
  radiobutton   $w.dump.rn -text "Parse Design Only" -variable dump_vpi_none -value "none" -anchor w -command "handle_new_cdd_dump_states $w"
  radiobutton   $w.dump.rd -text "Dumpfile:"         -variable dump_vpi_none -value "dump" -anchor w -command "handle_new_cdd_dump_states $w"
  entry         $w.dump.ed -textvariable dump_file -state disabled
  button        $w.dump.bd -text "Browse" -width 10 -state disabled -command {
    set dump_file [tk_getOpenFile -title "Select VCD/LXT Dumpfile" -filetypes $dump_filetypes]
  }
  radiobutton   $w.dump.rv -text "VPI Module:" -variable dump_vpi_none -value "vpi" -anchor w -command "handle_new_cdd_dump_states $w"
  entry         $w.dump.ev -textvariable vpi_file -state disabled
  tk_optionMenu $w.dump.mv simulator {Icarus Verilog} {Cver} {VCS}
  frame         $w.dump.ts
  checkbutton   $w.dump.ts.cb -text "Set VPI Module Timescale:" -anchor w -variable vpi_ts -state disabled -command "handle_new_cdd_dump_vpi_timescale $w"
  tk_optionMenu $w.dump.ts.n1 vpi_ts_num1   {1} {10} {100}
  tk_optionMenu $w.dump.ts.s1 vpi_ts_scale1 {s} {ms} {us} {ns} {ps} {fs}
  label         $w.dump.ts.l  -text "/" -state disabled
  tk_optionMenu $w.dump.ts.n2 vpi_ts_num2   {1} {10} {100}
  tk_optionMenu $w.dump.ts.s2 vpi_ts_scale2 {s} {ms} {us} {ns} {ps} {fs}
  pack $w.dump.ts.cb -side left -padx 3 -pady 3
  pack $w.dump.ts.n1 -side left -padx 3 -pady 3
  pack $w.dump.ts.s1 -side left -padx 3 -pady 3
  pack $w.dump.ts.l  -side left -padx 3 -pady 3
  pack $w.dump.ts.n2 -side left -padx 3 -pady 3
  pack $w.dump.ts.s2 -side left -padx 3 -pady 3

  $w.dump.mv    configure -state disabled
  $w.dump.ts.n1 configure -state disabled
  $w.dump.ts.s1 configure -state disabled
  $w.dump.ts.n2 configure -state disabled
  $w.dump.ts.s2 configure -state disabled

  grid columnconfig $w.dump 1 -weight 1
  grid $w.dump.rn -row 0 -column 0 -sticky news -padx 3 -pady 3 -columnspan 2
  grid $w.dump.rd -row 1 -column 0 -sticky news -padx 3 -pady 3
  grid $w.dump.ed -row 1 -column 1 -sticky news -padx 3 -pady 3
  grid $w.dump.bd -row 1 -column 2 -sticky news -padx 3 -pady 3
  grid $w.dump.rv -row 2 -column 0 -sticky news -padx 3 -pady 3
  grid $w.dump.ev -row 2 -column 1 -sticky news -padx 3 -pady 3
  grid $w.dump.mv -row 2 -column 2 -sticky news -padx 3 -pady 3
  grid $w.dump.ts -row 3 -column 0 -sticky news -padx 3 -pady 3 -columnspan 3

  # Create button bar
  frame  $w.bf
  button $w.bf.help   -width 10 -text "Help" -command {
    puts "TBD"
  }
  button $w.bf.cancel -width 10 -text "Cancel" -command "destroy [winfo toplevel $w]"
  button $w.bf.next   -width 10 -text "Next"   -command "goto_next_pane $w" -state disabled
  button $w.bf.prev   -width 10 -text "Back"   -command "goto_prev_pane $w"
  pack $w.bf.help   -side right -padx 4 -pady 4
  pack $w.bf.cancel -side right -padx 4 -pady 4
  pack $w.bf.next   -side right -padx 4 -pady 4
  pack $w.bf.prev   -side left  -padx 4 -pady 4

  # Pack frames
  pack $w.fu   -fill both -expand 1
  pack $w.dump -fill x
  pack $w.fl   -fill both -expand 1
  pack $w.bf   -fill x

  return $w

}

proc create_new_cdd_parse_race_pragma_name_cmd {w} {

  global race_cond_pragma

  if {$race_cond_pragma == 0} {
    $w.parse.race_e configure -state disabled
  } else {
    $w.parse.race_e configure -state normal
  }
  
}

proc create_new_cdd_parse_exclude_pragma_cmd {w} {

  global exclude_pragma

  if {$exclude_pragma == 0} {
    $w.exclude.epe configure -state disabled
  } else {
    $w.exclude.epe configure -state normal
  }

}

proc handle_new_cdd_parse_top_name_cmd {w} {

  global toplevel_name

  if {$toplevel_name ne ""} {
    $w.bf.next configure -state normal
  } else {
    $w.bf.next configure -state disabled
  }

  return 1

}

proc create_new_cdd_parse {w} {

  global toplevel_name inst_name delay_type
  global race_cond_action race_cond_pragma race_cond_pragma_name
  global assert_ovl
  global design_generation
  global exclude_always exclude_assign exclude_initial exclude_final exclude_pragma exclude_pragma_name

  # Create top-most frame
  frame $w
  labelframe $w.parse -text "General Options"

  # Add toplevel design name widgets
  label $w.parse.top_l -text "Toplevel module name:"
  entry $w.parse.top_e -textvariable toplevel_name -validate all
  
  # Add root pathname widgets
  label $w.parse.inst_l -text "Root pathname:"
  entry $w.parse.inst_e -textvariable inst_name
  
  # Add delay specification
  label         $w.parse.delay_l -text "Delay Type:"
  tk_optionMenu $w.parse.delay_m delay_type None Min Typ Max
  
  # Add race condition option
  label         $w.parse.race_l  -text "Race Condition Action:"
  tk_optionMenu $w.parse.race_m race_cond None Silent Warning Error Ignore
  checkbutton   $w.parse.race_cb -text "Use embedded race condition pragmas" -variable race_cond_pragma
  entry         $w.parse.race_e  -state disabled -textvariable race_cond_pragma_name
  $w.parse.race_cb configure -command "create_new_cdd_parse_race_pragma_name_cmd $w"
  
  # Add generation options
  label         $w.parse.gen_l -text "Default Verilog Generation:"
  tk_optionMenu $w.parse.gen_m design_generation {Verilog 1995} {Verilog 2001} {System Verilog}

  # Add assertion options
  checkbutton $w.parse.assert_ovl -text "Include OVL Assertions" -variable assert_ovl -anchor w
  
  # Create a grid for the above widgets
  grid columnconfigure $w.parse 3 -weight 1
  grid $w.parse.top_l      -row 0 -column 0 -padx 3 -pady 3 -sticky nw
  grid $w.parse.top_e      -row 0 -column 1 -padx 3 -pady 3 -sticky news -columnspan 3
  grid $w.parse.inst_l     -row 1 -column 0 -padx 3 -pady 3 -sticky nw
  grid $w.parse.inst_e     -row 1 -column 1 -padx 3 -pady 3 -sticky news -columnspan 3
  grid $w.parse.delay_l    -row 2 -column 0 -padx 3 -pady 3 -sticky nw
  grid $w.parse.delay_m    -row 2 -column 1 -padx 3 -pady 3 -sticky nw
  grid $w.parse.race_l     -row 3 -column 0 -padx 3 -pady 3 -sticky nw
  grid $w.parse.race_m     -row 3 -column 1 -padx 3 -pady 3 -sticky nw
  grid $w.parse.race_cb    -row 3 -column 2 -padx 3 -pady 3 -sticky news
  grid $w.parse.race_e     -row 3 -column 3 -padx 3 -pady 3 -sticky news
  grid $w.parse.gen_l      -row 4 -column 0 -padx 3 -pady 3 -sticky nw
  grid $w.parse.gen_m      -row 4 -column 1 -padx 3 -pady 3 -sticky nw
  grid $w.parse.assert_ovl -row 5 -column 0 -padx 3 -pady 3 -sticky news -columnspan 4

  # Add exclusion options
  labelframe  $w.exclude -text "Exclude Options"
  checkbutton $w.exclude.ea -anchor w -text "Exclude Always Blocks"  -variable exclude_always
  checkbutton $w.exclude.ec -anchor w -text "Exclude Assign Blocks"  -variable exclude_assign
  checkbutton $w.exclude.ei -anchor w -text "Exclude Initial Blocks" -variable exclude_initial
  checkbutton $w.exclude.ef -anchor w -text "Exclude Final Blocks"   -variable exclude_final
  checkbutton $w.exclude.epc -text "Exclude Within Pragma:" -variable exclude_pragma
  entry       $w.exclude.epe -state disabled -textvariable exclude_pragma_name
  $w.exclude.epc configure -command "create_new_cdd_parse_exclude_pragma_cmd $w"
  
  grid $w.exclude.ea  -row 0 -column 0 -sticky news -padx 3 -pady 3
  grid $w.exclude.ec  -row 0 -column 1 -sticky news -padx 3 -pady 3
  grid $w.exclude.ei  -row 1 -column 0 -sticky news -padx 3 -pady 3
  grid $w.exclude.ef  -row 1 -column 1 -sticky news -padx 3 -pady 3
  grid $w.exclude.epc -row 2 -column 0 -sticky news -padx 3 -pady 3
  grid $w.exclude.epe -row 2 -column 1 -sticky news -padx 3 -pady 3

  # Create button bar
  frame  $w.bf
  button $w.bf.help   -width 10 -text "Help" -command {
    puts "TBD"
  }
  button $w.bf.cancel -width 10 -text "Cancel" -command "destroy [winfo toplevel $w]"
  button $w.bf.next   -width 10 -text "Next"   -command "goto_next_pane $w" -state disabled
  button $w.bf.prev   -width 10 -text "Back"   -command "goto_prev_pane $w"
  pack $w.bf.help   -side right -padx 4 -pady 4
  pack $w.bf.cancel -side right -padx 4 -pady 4
  pack $w.bf.next   -side right -padx 4 -pady 4
  pack $w.bf.prev   -side left  -padx 4 -pady 4

  $w.parse.top_e configure -vcmd "handle_new_cdd_parse_top_name_cmd $w"

  # Pack frames
  pack $w.parse   -fill x -pady 10
  pack $w.exclude -fill x -pady 10
  pack $w.bf      -side bottom -fill x

  return $w

}

proc handle_new_cdd_parse2_source_file {w} {

  set value [tk_getOpenFile -title "Select Verilog Source File"]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Source File" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Source File" $value]
    }

  }

}

proc handle_new_cdd_parse2_lib_dir {w} {

  set value [tk_chooseDirectory -title "Select Verilog Library Directory" -mustexist true]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Library Directory" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Library Directory" $value]
    }

  }

}

proc handle_new_cdd_parse2_lib_ext {w} {

  set value [get_library_extensions ""]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Library Extension" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Library Extension" $value]
    }

  }

}

proc handle_new_cdd_parse2_inc_dir {w} {

  set value [tk_chooseDirectory -title "Select Include Directory" -mustexist true]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Include Directory" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Include Directory" $value]
    }

  }

}

proc handle_new_cdd_parse2_define {w} {

  set value [get_define "" ""]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Define" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Define" $value]
    }

  }

}

proc handle_new_cdd_parse2_parm_oride {w} {

  set value [get_parameter_override "" ""]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Param Override" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Param Override" $value]
    }

  }

}

proc handle_new_cdd_parse2_fsm {w} {

  set value [get_fsm "" "" ""]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "FSM" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "FSM" $value]
    }

  }

}

proc handle_new_cdd_parse2_mod_gen {w} {

  set value [get_module_generation "" ""]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Module Generation" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Module Generation" $value]
    }

  }

}

proc handle_new_cdd_parse2_mod_excl {w} {

  set value [get_module_exclusion ""]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Module Exclusion" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Module Exclusion" $value]
    }

  }

}

proc handle_new_cdd_parse2_cmd_file {w} {

  set value [tk_getOpenFile -title "Select Command File"]

  if {$value ne ""} {

    set index [$w.opts.lbf.lb curselection]

    if {$index eq ""} {
      $w.opts.lbf.lb insert end [list "Command File" $value]
    } else {
      $w.opts.lbf.lb insert $index [list "Command File" $value]
    }

  }

}

proc handle_new_cdd_parse2_edit {w} {

  set index  [$w.opts.lbf.lb curselection]
  set values [$w.opts.lbf.lb get $index]
  set type   [lindex $values 0]
  set value  [lindex $values 1]

  if {$type eq "Source File"} {
    set value [tk_getOpenFile -title "Select Verilog Source File" -initialfile $value]
  } elseif {$type eq "Library Directory"} {
    set value [tk_chooseDirectory -title "Select Verilog Library Directory" -mustexist true -initialdir $value]
  } elseif {$type eq "Library Extension"} {
    set value [get_library_extensions [lrange [split $value "+"] 1 end-1]]
  } elseif {$type eq "Include Directory"} {
    set value [tk_chooseDirectory -title "Select Include Directory" -mustexist true -initialdir $value]
  } elseif {$type eq "Command File"} {
    set value [tk_getOpenFile -title "Select Command File" -initialfile $value]
  } elseif {$type eq "Module Exclusion"} {
    set value [get_module_exclusion $value]
  } elseif {$type eq "Module Generation"} {
    set old_value [split $value "="]
    set value [get_module_generation [lindex $old_value 0] [lindex $old_value 1]]
  } elseif {$type eq "Define"} {
    set old_value [split $value "="]
    set value [get_define [lindex $old_value 0] [lindex $old_value 1]]
  } elseif {$type eq "Param Override"} {
    set old_value [split $value "="]
    set value [get_parameter_override [lindex $old_value 0] [lindex $old_value 1]]
  } elseif {$type eq "FSM"} {
    set old_value1 [split $value "="]
    set old_value2 [split [lindex $old_value1 1] ","]
    set value [get_fsm [lindex $old_value1 0] [lindex $old_value2 0] [lindex $old_value2 1]]
  }

  set index [$w.opts.lbf.lb curselection]

  if {$value ne ""} {
    $w.opts.lbf.lb delete $index
    $w.opts.lbf.lb insert $index [list $type $value]
    $w.opts.lbf.lb selection set $index
  }

}

proc handle_new_cdd_parse2_lb {w} {

  if {[$w.opts.lbf.lb curselection] ne ""} {
    $w.opts.bf.edit   configure -state normal
    $w.opts.bf.delete configure -state normal
  }

}

proc create_new_cdd_parse2 {w} {

  frame $w
  frame $w.opts

  # Create and pack the listbox frame
  frame     $w.opts.lbf
  tablelist::tablelist $w.opts.lbf.lb -exportselection 0 -movablerows 1 -selectmode single -columns {0 "Type" 0 "Argument"} \
    -labelcommand tablelist::sortByColumn -xscrollcommand "$w.opts.lbf.hb set" -yscrollcommand "$w.opts.lbf.vf.vb set" -stretch all
  frame     $w.opts.lbf.vf
  label     $w.opts.lbf.vf.l
  scrollbar $w.opts.lbf.vf.vb -command "$w.opts.lbf.lb yview" -takefocus 0
  pack      $w.opts.lbf.vf.l  -fill x
  pack      $w.opts.lbf.vf.vb -fill y -expand 1
  scrollbar $w.opts.lbf.hb -orient horizontal -command "$w.opts.lbf.lb xview" -takefocus 0

  grid rowconfigure    $w.opts.lbf 0 -weight 1
  grid columnconfigure $w.opts.lbf 0 -weight 1
  grid $w.opts.lbf.lb -row 0 -column 0 -sticky news
  grid $w.opts.lbf.vf -row 0 -column 1 -sticky ns
  grid $w.opts.lbf.hb -row 1 -column 0 -sticky ew

  # Create and pack the button frame
  frame       $w.opts.bf
  menubutton  $w.opts.bf.ins_mb -text "Insert" -relief raised -highlightthickness 1 -takefocus 1
  set m [menu $w.opts.bf.ins_mb.m -tearoff false -takefocus 1]
  $w.opts.bf.ins_mb configure -menu $m
  bind $w.opts.bf.ins_mb <Return> "
    $w.opts.bf.ins_mb.m post [winfo rootx $w.opts.bf.ins_mb] [winfo rooty $w.opts.bf.ins_mb]
    $w.opts.bf.ins_mb.m activate 0
  "
  bind $w.opts.bf.ins_mb <FocusOut> "$w.opts.bf.ins_mb.m unpost"

  # Add the menu items
  $m add command -label "Source File..."          -command "handle_new_cdd_parse2_source_file $w"
  $m add command -label "Library Directory..."    -command "handle_new_cdd_parse2_lib_dir $w"
  $m add command -label "Library Extension(s)..." -command "handle_new_cdd_parse2_lib_ext $w"
  $m add command -label "Include Directory..."    -command "handle_new_cdd_parse2_inc_dir $w"
  $m add command -label "Define..."               -command "handle_new_cdd_parse2_define $w"
  $m add command -label "Parameter Override..."   -command "handle_new_cdd_parse2_parm_oride $w"
  $m add separator
  $m add command -label "FSM..."                  -command "handle_new_cdd_parse2_fsm $w"
  $m add command -label "Module Generation..."    -command "handle_new_cdd_parse2_mod_gen $w"
  $m add command -label "Module Exclusion..."     -command "handle_new_cdd_parse2_mod_excl $w"
  $m add separator
  $m add command -label "Command File..."         -command "handle_new_cdd_parse2_cmd_file $w"

  button $w.opts.bf.edit   -text "Edit..." -width 10 -state disabled -command "handle_new_cdd_parse2_edit $w"
  button $w.opts.bf.delete -text "Delete"  -width 10 -state disabled -command "$w.opts.lbf.lb delete [$w.opts.lbf.lb curselection]"
  bind $w.opts.bf.edit <Return> {%W invoke}
  bind $w.opts.bf.delete <Return> {%W invoke}
  bind $w.opts.lbf.lb <<ListboxSelect>> "handle_new_cdd_parse2_lb $w"
  pack $w.opts.bf.ins_mb -fill x -padx 3 -pady 3
  pack $w.opts.bf.edit   -fill x -padx 3 -pady 3
  pack $w.opts.bf.delete -fill x -padx 3 -pady 3

  # Now pack the buttons and listbox frames in the labelframe
  pack $w.opts.lbf -side left  -fill both -expand 1
  pack $w.opts.bf  -side right -fill y

  # Create command file save button
  frame $w.save
  button $w.save.b -text "Save Options to File..." -command {
    set cddgen_sname [tk_getSaveFile -title "Save Score Options to File..." -initialfile $cddgen_sname -parent .newwin]
    if {$cddgen_sname ne ""} {
      if {[catch {set fp [open $cddgen_sname "w"]}]} {
        tk_messageBox -message "File $sname Not Writable!" -title "No File" -icon error
      }
      foreach arg [create_arglist_for_new_cdd] {
        puts $fp "$arg"
      }
      close $fp
    }
  }
  pack $w.save.b -pady 4

  # Create button bar
  frame  $w.bf
  button $w.bf.help   -width 10 -text "Help" -command {
    puts "TBD"
  } 
  button $w.bf.cancel -width 10 -text "Cancel"   -command "destroy [winfo toplevel $w]"
  button $w.bf.gen    -width 10 -text "Generate" -command "generate_new_cdd $w"
  button $w.bf.prev   -width 10 -text "Back"     -command "goto_prev_pane $w" 
  pack $w.bf.help   -side right -padx 4 -pady 4
  pack $w.bf.cancel -side right -padx 4 -pady 4
  pack $w.bf.gen    -side right -padx 4 -pady 4
  pack $w.bf.prev   -side left  -padx 4 -pady 4

  # Pack frames
  pack $w.opts -fill both -expand 1
  pack $w.save -fill x
  pack $w.bf   -fill x -side bottom

  return $w

}

proc create_new_cdd_finish {w} {

  global cddgen_auto_open cdd_filename

  if {$cddgen_auto_open == 1} {
    open_files [list $cdd_filename]
  }

  destroy [winfo toplevel $w]

}

proc create_new_cdd_output {w} {

  global cddgen_auto_open

  # Create top-most frame
  frame $w

  # Create output textbox and associated scrollbars
  frame     $w.f
  text      $w.f.t -state disabled -xscrollcommand "$w.f.hb set" -yscrollcommand "$w.f.vb set"
  scrollbar $w.f.vb -command "$w.f.t yview" -takefocus 0
  scrollbar $w.f.hb -orient horizontal -command "$w.f.t.xview" -takefocus 0
  grid rowconfigure    $w.f 0 -weight 1
  grid columnconfigure $w.f 0 -weight 1
  grid $w.f.t  -row 0 -column 0 -sticky news
  grid $w.f.vb -row 0 -column 1 -sticky ns
  grid $w.f.hb -row 1 -column 0 -sticky ew

  # Create open checkbutton
  frame       $w.fo
  checkbutton $w.fo.cb -text "Open/Merge the newly create CDD" -state disabled -variable cddgen_auto_open
  pack $w.fo.cb -pady 3

  # Create button bar
  frame  $w.bf
  button $w.bf.help   -width 10 -text "Help" -command {
    puts "TBD"
  }
  button $w.bf.finish -width 10 -text "Finish" -command "create_new_cdd_finish $w"
  button $w.bf.prev   -width 10 -text "Back"   -command "goto_prev_pane $w"
  pack   $w.bf.help   -side right -padx 4 -pady 4
  pack   $w.bf.finish -side right -padx 4 -pady 4
  pack   $w.bf.prev   -side left  -padx 4 -pady 4

  # Pack frames
  pack $w.f  -fill both -expand 1
  pack $w.fo -fill x
  pack $w.bf -fill x -side bottom

  return $w

}

proc set_widget_state {name state} {

  global new_widget_state

  set children [winfo children $name]

  foreach child $children {
    set_widget_state $child $state
  }

  # If this widget has a -state option, set it to the given value
  if {[lsearch -exact [join [$name configure]] {-state}] != -1} {
    if {$state eq "disabled"} {
      if {[$name cget -state] ne "disabled"} {
        set new_widget_state($name) [$name cget -state]
      }
    } else {
      if {[info exists new_widget_state($name)]} {
        set state $new_widget_state($name)
      } else {
        set new_widget_state($name) [$name cget -state]
      }
    }
    if {$state ne ""} {
      $name configure -state $state
    }
  }

}

proc get_module_generation {modname gen} {

  global mod_gen_retval mod_gen_modname

  set mod_gen_retval  ""
  set mod_gen_modname $modname

  # Create new window
  toplevel .mgenwin
  wm title .mgenwin "Specify generation for module"

  # Add selection widgets
  frame .mgenwin.f -relief raised
  label .mgenwin.f.l -text "Module name:"
  entry .mgenwin.f.e -textvariable mod_gen_modname -validate all
  tk_optionMenu .mgenwin.f.m modgen "Verilog 1995" "Verilog 2001" "System Verilog"
  pack .mgenwin.f.l -side left  -padx 8 -pady 4 -fill x
  pack .mgenwin.f.e -side left  -padx 8 -pady 4 -fill x -expand 1
  pack .mgenwin.f.m -side right -padx 8 -pady 4 -fill x

  # Add button frame widgets
  frame  .mgenwin.bf -relief raised
  button .mgenwin.bf.ok -text "OK" -width 10 -state disabled -command {
    if {$modgen eq "Verilog 1995"} {
      set mod_gen_retval "$mod_gen_modname=1"
    } elseif {$modgen eq "Verilog 2001"} {
      set mod_gen_retval "$mod_gen_modname=2"
    } elseif {$modgen eq "System Verilog"} {
      set mod_gen_retval "$mod_gen_modname=3"
    }
    destroy .mgenwin
  }
  bind .mgenwin.bf.ok <Return> {%W invoke}
  button .mgenwin.bf.cancel -text "Cancel" -width 10 -command {
    set mod_gen_retval ""
    destroy .mgenwin
  }
  bind .mgenwin.bf.cancel <Return> {%W invoke}
  pack .mgenwin.bf.cancel -side right -padx 8 -pady 4
  pack .mgenwin.bf.ok     -side right -padx 8 -pady 4

  # Add functionality for entry
  .mgenwin.f.e configure -vcmd {
    if {$mod_gen_modname ne ""} {
      .mgenwin.bf.ok configure -state normal
    } else {
      .mgenwin.bf.ok configure -state disabled
    }
    return 1
  }

  # Pack the window
  pack .mgenwin.f  -fill x
  pack .mgenwin.bf -fill x

  # Make sure that this window is a transient window and set focus
  wm transient .mgenwin .newwin
  focus .mgenwin.f.e

  # Wait for the module generation window to be destroyed before returning
  tkwait window .mgenwin

  return $mod_gen_retval

}

proc get_module_exclusion {modname} {

  global mod_excl_retval mod_excl_modname

  set mod_excl_retval  ""
  set mod_excl_modname $modname

  # Create new window
  toplevel .mexclwin
  wm title .mexclwin "Specify module to exclude"

  # Add selection widgets
  frame .mexclwin.f -relief raised
  label .mexclwin.f.l -text "Module name:"
  entry .mexclwin.f.e -textvariable mod_excl_modname -validate all
  pack .mexclwin.f.l -side left -padx 8 -pady 4 -fill x
  pack .mexclwin.f.e -side left -padx 8 -pady 4 -fill x -expand 1

  # Add button frame widgets
  frame .mexclwin.bf -relief raised
  button .mexclwin.bf.ok -text "OK" -width 10 -state disabled -command {
    set mod_excl_retval "$mod_excl_modname"
    destroy .mexclwin
  }
  bind .mexclwin.bf.ok <Return> {%W invoke}
  button .mexclwin.bf.cancel -text "Cancel" -width 10 -command {
    set mod_excl_retval ""
    destroy .mexclwin
  }
  bind .mexclwin.bf.cancel <Return> {%W invoke}
  pack .mexclwin.bf.cancel -side right -padx 8 -pady 4
  pack .mexclwin.bf.ok     -side right -padx 8 -pady 4

  # Provide functionality for the entry
  .mexclwin.f.e configure -vcmd {
    if {$mod_excl_modname ne ""} {
      .mexclwin.bf.ok configure -state normal
    } else {
      .mexclwin.bf.ok configure -state disabled
    }
    return 1
  }

  # Pack the window
  pack .mexclwin.f  -fill x
  pack .mexclwin.bf -fill x

  # Make sure that this window is transient and sets focus to first widget
  wm transient .mexclwin .newwin
  focus .mexclwin.f.e

  # Wait for the module exclusion window to be destroyed before returning
  tkwait window .mexclwin

  return $mod_excl_retval

}

proc get_library_extensions {extensions} {

  global lib_ext_retval
  global lib_ext_name

  set lib_ext_retval ""
  set lib_ext_name   ""

  # Create new window
  toplevel .lextwin
  wm title .lextwin "Specify library extensions"

  # Add selection widgets
  frame     .lextwin.f
  frame     .lextwin.f.ef
  entry     .lextwin.f.ef.e -validate all -textvariable lib_ext_name
  button    .lextwin.f.ef.b -text "Update" -width 10 -state disabled
  pack .lextwin.f.ef.e -side left  -fill x -expand 1 -padx 3 -pady 3
  pack .lextwin.f.ef.b -side right -padx 3 -pady 3
  frame     .lextwin.f.lf
  listbox   .lextwin.f.lf.lb -xscrollcommand {.lextwin.f.lf.hb set} -yscrollcommand {.lextwin.f.lf.vb set} -exportselection 0 -selectmode single
  scrollbar .lextwin.f.lf.hb -orient horizontal -command {.lextwin.f.lf.lb xset} -takefocus 0
  scrollbar .lextwin.f.lf.vb -command {.lextwin.f.lf.lb yset} -takefocus 0
  grid rowconfigure    .lextwin.f.lf 0 -weight 1
  grid columnconfigure .lextwin.f.lf 0 -weight 1
  grid .lextwin.f.lf.lb -row 0 -column 0 -sticky news -padx 3 -pady 3
  grid .lextwin.f.lf.vb -row 0 -column 1 -sticky ns   -padx 3 -pady 3
  grid .lextwin.f.lf.hb -row 1 -column 0 -sticky ew   -padx 3 -pady 3
  pack .lextwin.f.ef -fill x    -expand 1 -padx 3 -pady 3
  pack .lextwin.f.lf -fill both -expand 1 -padx 3 -pady 3

  # Update the listbox with any previous values
  .lextwin.f.lf.lb insert end "Click to Add New Extension"
  foreach extension $extensions {
    if {$extension eq ""} {
      .lextwin.f.lf.lb insert end "<None>"
    } else {
      .lextwin.f.lf.lb insert end $extension
    }
  }
  .lextwin.f.lf.lb selection set 0

  # Create bindings
  bind .lextwin.f.lf.lb <<ListboxSelect>> {
    set index [.lextwin.f.lf.lb curselection]
    if {$index ne ""} {
      if {[.lextwin.f.lf.lb get $index] eq "Click to Add New Extension"} {
        set lib_ext_name ""
      } elseif {[.lextwin.f.lf.lb get $index] eq "<None>"} {
        set lib_ext_name ""
        .lextwin.f.lf.lb delete $index
      } else {
        set lib_ext_name [.lextwin.f.lf.lb get $index]
        .lextwin.f.lf.lb delete $index
      }
      .lextwin.f.lf.lb selection set 0
      .lextwin.f.ef.e  selection range 0 end
      .lextwin.f.ef.e  icursor end
    }
  }
  .lextwin.f.ef.e configure -vcmd {
    # See if the current name exists in the listbox
    if {[llength %P] == 0} {
      set curr_val "<None>"
    } else {
      set curr_val "%P"
    }
    for {set i 1} {$i < [.lextwin.f.lf.lb size]} {incr i} {
      if {[.lextwin.f.lf.lb get $i] eq $curr_val} {
        break
      }
    }
    if {$i == [.lextwin.f.lf.lb size]} {
      if {[llength %P] == 0} {
        .lextwin.f.ef.b configure -state normal
      } elseif {[string index "%P" 0] eq "."} {
        .lextwin.f.ef.b configure -state normal
      } else {
        .lextwin.f.ef.b configure -state disabled
      }
    } else {
      .lextwin.f.ef.b configure -state disabled
    }
    focus .lextwin.f.ef.e
    return 1
  }
  .lextwin.f.ef.b configure -command {
    if {$lib_ext_name eq ""} {
      .lextwin.f.lf.lb insert end "<None>"
    } else {
      .lextwin.f.lf.lb insert end $lib_ext_name
    }
    .lextwin.f.lf.lb selection set 0
    set lib_ext_name ""
  }
  bind .lextwin.f.ef.e <Return> {.lextwin.f.ef.b invoke}

  # Add button frame and buttons
  frame  .lextwin.bf
  button .lextwin.bf.ok -text "OK" -width 10 -command {
    set lib_ext_retval "+"
    for {set i 1} {$i < [.lextwin.f.lf.lb size]} {incr i} {
      if {[.lextwin.f.lf.lb get $i] eq "<None>"} {
        set lib_ext_retval "$lib_ext_retval+"
      } else {
        set lib_ext_retval "$lib_ext_retval[.lextwin.f.lf.lb get $i]+"
      }
    }
    destroy .lextwin
  }
  button .lextwin.bf.cancel -text "Cancel" -width 10 -command {
    set lib_ext_retval ""
    destroy .lextwin
  }
  pack .lextwin.bf.cancel -side right -padx 3 -pady 3
  pack .lextwin.bf.ok     -side right -padx 3 -pady 3

  # Pack the window
  pack .lextwin.f  -fill both -expand 1
  pack .lextwin.bf -fill x

  # Make sure that this window is transient and sets focus to first widget
  wm transient .lextwin .newwin
  focus .lextwin.f.ef.e

  # Wait for the module exclusion window to be destroyed before returning
  tkwait window .lextwin

  return $lib_ext_retval

}

proc define_update_state {} {

  global define_name define_value

  if {$define_name ne ""} {
    if {[.defwin.f.b cget -relief] eq "raised"} {
      .defwin.bf.ok configure -state normal
    } elseif {$define_value ne ""} {
      .defwin.bf.ok configure -state normal
    } else {
      .defwin.bf.ok configure -state disabled
    }
  } else {
    .defwin.bf.ok configure -state disabled
  }

  return 1

}

proc get_define {defname value} {

  global def_retval define_name define_value

  set def_retval   ""
  set define_name  $defname
  set define_value $value

  # Create new window
  toplevel .defwin
  wm title .defwin "Specify a command-line define value"

  # Add selection widgets
  frame .defwin.f
  label .defwin.f.l -text "Define name:"
  entry .defwin.f.e -textvariable define_name -validate all
  entry .defwin.f.v -state disabled -textvariable define_value -validate all
  button .defwin.f.b -text "=" -width 1 -relief raised -command {
    if {[.defwin.f.b cget -relief] eq "raised"} {
      .defwin.f.v configure -state normal
      .defwin.f.b configure -relief flat
      if {$define_value eq ""} {
        .defwin.bf.ok configure -state disabled
      }
    } else {
      .defwin.f.v delete 0 end
      .defwin.f.v configure -state disabled
      .defwin.f.b configure -relief raised
    }
  }
  bind .defwin.f.b <Return> {%W invoke}

  # If a value was specified, enable the textbox and set the relief to flat on the '=' button
  if {$value ne ""} {
    .defwin.f.v configure -state normal
    .defwin.f.b configure -relief flat
  }
  
  # Pack the input widgets
  grid rowconfigure    .defwin.f 1 -weight 1
  grid columnconfigure .defwin.f 3 -weight 1
  grid .defwin.f.l -row 0 -column 0 -sticky news -pady 4 -padx 8
  grid .defwin.f.e -row 0 -column 1 -sticky news -pady 4 -padx 8
  grid .defwin.f.b -row 0 -column 2 -sticky news -pady 4 -padx 8
  grid .defwin.f.v -row 0 -column 3 -sticky news -pady 4 -padx 8

  # Add button frame and widgets
  frame .defwin.bf
  button .defwin.bf.ok -text "OK" -width 10 -state disabled -command {
    if {[.defwin.f.b cget -relief] eq "raised"} {
      set def_retval "$define_name"
    } else {
       set def_retval "$define_name=$define_value"
    }
    destroy .defwin
  }
  bind .defwin.bf.ok <Return> {%W invoke}
  button .defwin.bf.cancel -text "Cancel" -width 10 -pady 4 -padx 8 -command {
    set def_retval ""
    destroy .defwin
  }
  bind .defwin.bf.cancel <Return> {%W invoke}
  pack .defwin.bf.cancel -side right -padx 8 -pady 4
  pack .defwin.bf.ok     -side right -padx 8 -pady 4

  # Provide functionality for entries
  .defwin.f.e configure -vcmd { return [define_update_state] }
  .defwin.f.v configure -vcmd { return [define_update_state] }

  # Pack the frames
  pack .defwin.f  -fill x
  pack .defwin.bf -fill x

  # Make sure that this window is transient and sets the first focus
  wm transient .defwin .newwin
  focus .defwin.f.e

  # Wait for the define window to be destroyed before returning
  tkwait window .defwin

  return $def_retval
  
}

proc parameter_update_state {} {

  global param_name param_value
 
  if {$param_name ne ""} {
    if {$param_value ne ""} {
      .parmwin.bf.ok configure -state normal
    } else {
      .parmwin.bf.ok configure -state disabled
    }
  } else {
    .parmwin.bf.ok configure -state disabled
  }

  return 1

}

proc get_parameter_override {parmname value} {

  global param_retval param_name param_value

  set param_retval ""
  set param_name   $parmname
  set param_value  $value

  # Create new window
  toplevel .parmwin
  wm title .parmwin "Specify a command-line parameter override"

  # Add selection widgets
  frame .parmwin.f -relief raised
  label .parmwin.f.l1 -text "Parameter scope:"
  entry .parmwin.f.e  -textvariable param_name -validate all
  label .parmwin.f.l2 -text "="
  entry .parmwin.f.v  -textvariable param_value -validate all

  # Pack the input widgets
  pack .parmwin.f.l1 -padx 8 -pady 4 -side left -fill x
  pack .parmwin.f.e  -padx 8 -pady 4 -side left -fill x -expand 1
  pack .parmwin.f.l2 -padx 8 -pady 4 -side left -fill x
  pack .parmwin.f.v  -padx 8 -pady 4 -side left -fill x -expand 1

  # Add button frame and widgets
  frame .parmwin.bf -relief raised
  button .parmwin.bf.ok -text "OK" -width 10 -state disabled -command {
    set param_retval "$param_name=$param_value"
    destroy .parmwin
  }
  bind .parmwin.bf.ok <Return> {%W invoke}
  button .parmwin.bf.cancel -text "Cancel" -width 10 -command {
    set param_retval ""
    destroy .parmwin
  }
  bind .parmwin.bf.cancel <Return> {%W invoke}
  pack .parmwin.bf.cancel -side right -padx 8 -pady 4
  pack .parmwin.bf.ok     -side right -padx 8 -pady 4

  # Provide entry functionality
  .parmwin.f.e configure -vcmd { return [parameter_update_state] }
  .parmwin.f.v configure -vcmd { return [parameter_update_state] }

  # Pack the frames
  pack .parmwin.f  -fill x
  pack .parmwin.bf -fill x

  # Make sure that this window is transient and sets the first focus
  wm transient .parmwin .newwin
  focus .parmwin.f.e

  # Wait for the parameter window to be destroyed before returning
  tkwait window .parmwin

  return $param_retval

}

proc fsmd_update_state {} {

  global fsmd_modname fsmd_output

  if {$fsmd_modname ne ""} {
    if {$fsmd_output ne ""} {
      .fsmdwin.bf.ok configure -state normal
    } else {
      .fsmdwin.bf.ok configure -state disabled
    }
  } else {
    .fsmdwin.bf.ok configure -state disabled
  }

  return 1

}

proc get_fsm {modname input output} {

  global fsmd_retval
  global fsmd_modname fsmd_input fsmd_output

  set fsmd_retval  ""
  set fsmd_modname $modname
  set fsmd_input   $input
  set fsmd_output  $output

  # Create new window
  toplevel .fsmdwin
  wm title .fsmdwin "Specify an FSM input/output state"

  # Add input widgets
  frame .fsmdwin.f -relief raised
  label .fsmdwin.f.l -text "Module name:"
  entry .fsmdwin.f.e -textvariable fsmd_modname -validate all
  label .fsmdwin.f.lo -text "Output Expression:"
  button .fsmdwin.f.b -text "Input == Output" -command {
    set fsmd_input $fsmd_output
  }
  bind .fsmdwin.f.b <Return> {%W invoke}
  entry .fsmdwin.f.eo -textvariable fsmd_output -validate all
  label .fsmdwin.f.li -text "Input Expression:"
  entry .fsmdwin.f.ei -textvariable fsmd_input

  grid columnconfigure .fsmdwin.f 1 -weight 1
  grid columnconfigure .fsmdwin.f 3 -weight 1
  grid .fsmdwin.f.l  -row 0 -column 0 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.e  -row 0 -column 1 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.lo -row 0 -column 2 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.eo -row 0 -column 3 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.b  -row 1 -column 1 -sticky e    -padx 8 -pady 4
  grid .fsmdwin.f.li -row 1 -column 2 -sticky news -padx 8 -pady 4
  grid .fsmdwin.f.ei -row 1 -column 3 -sticky news -padx 8 -pady 4

  # Add button frame and widgets
  frame .fsmdwin.bf -relief raised
  button .fsmdwin.bf.ok -text "OK" -width 10 -state disabled -command {
    set fsmd_retval "$fsmd_modname=$fsmd_input,$fsmd_output"
    destroy .fsmdwin
  }
  bind .fsmdwin.bf.ok <Return> {%W invoke}
  button .fsmdwin.bf.cancel -text "Cancel" -width 10 -command {
    set fsmd_retval ""
    destroy .fsmdwin
  }
  bind .fsmdwin.bf.cancel <Return> {%W invoke}
  pack .fsmdwin.bf.cancel -side right -padx 8 -pady 4
  pack .fsmdwin.bf.ok     -side right -padx 8 -pady 4

  # Provide functionality for entries
  .fsmdwin.f.e  configure -vcmd { return [fsmd_update_state] }
  .fsmdwin.f.eo configure -vcmd { return [fsmd_update_state] }

  # Pack frames
  pack .fsmdwin.f  -fill x
  pack .fsmdwin.bf -fill x
  
  # Make sure that this window is transient and sets the first focus
  wm transient .fsmdwin .newwin
  focus .fsmdwin.f.e

  # Wait for the parameter window to be destroyed before returning
  tkwait window .fsmdwin

  return $fsmd_retval

}
