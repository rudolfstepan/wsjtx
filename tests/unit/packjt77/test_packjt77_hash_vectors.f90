program test_packjt77_hash_vectors

  use packjt77
  implicit none

  integer :: ntests

  ntests=0

  call expect_hash_vector('K1ABC', 712, 2851, 2920267)
  call expect_hash_vector('W9XYZ', 972, 3889, 3982604)
  call expect_hash_vector('PJ4/K1ABC', 346, 1387, 1420834)
  call expect_hash_vector('KH1/KH7Z', 201, 806, 825805)
  call expect_hash_vector('K1JT/P', 959, 3839, 3931158)
  call expect_hash_vector('W3CCX', 665, 2662, 2726483)
  call expect_hash_vector('3DA0ABC', 48, 192, 196695)
  call expect_hash_vector('3XABC', 563, 2254, 2308721)
  call expect_hash_vector('///////////', 671, 2685, 2749801)
  call expect_hash_vector('ZZZZZZZZZZZ', 902, 3609, 3695718)
  call expect_hash_vector('99999999999', 762, 3050, 3123740)
  call expect_hash_vector('A23456789Z/', 122, 488, 499902)
  call expect_hash_vector('           ', 0, 0, 0)

  write(*,1000) ntests
1000 format('packjt77 hash vector tests passed: ',i0)

contains

  subroutine expect_hash_vector(callsign,want10,want12,want22)
    character(len=*), intent(in) :: callsign
    integer, intent(in) :: want10, want12, want22
    character(len=13) :: c13
    integer :: got10, got12, got22
    integer :: got10var, got12var, got22var

    c13='             '
    c13=callsign
    got10=ihashcall(c13,10)
    got12=ihashcall(c13,12)
    got22=ihashcall(c13,22)
    got10var=ihashcallvar(c13,10)
    got12var=ihashcallvar(c13,12)
    got22var=ihashcallvar(c13,22)

    if(got10.ne.want10 .or. got12.ne.want12 .or. got22.ne.want22) then
       write(*,1010) trim(callsign), got10, got12, got22, want10, want12, want22
1010   format('Hash vector failure for "',a,'"; got ',i0,1x,i0,1x,i0, &
              ' wanted ',i0,1x,i0,1x,i0)
       error stop 1
    endif

    if(got10var.ne.want10 .or. got12var.ne.want12 .or. got22var.ne.want22) then
       write(*,1020) trim(callsign), got10var, got12var, got22var, &
            want10, want12, want22
1020   format('Var hash vector failure for "',a,'"; got ',i0,1x,i0,1x,i0, &
              ' wanted ',i0,1x,i0,1x,i0)
       error stop 1
    endif

    if(got10.ne.got10var .or. got12.ne.got12var .or. got22.ne.got22var) then
       write(*,1030) trim(callsign), got10, got12, got22, got10var, got12var, got22var
1030   format('Hash helper mismatch for "',a,'"; standard ',i0,1x,i0,1x,i0, &
              ' var ',i0,1x,i0,1x,i0)
       error stop 1
    endif

    if(got10.lt.0 .or. got10.gt.1023 .or. got12.lt.0 .or. got12.gt.4095 .or. &
         got22.lt.0 .or. got22.gt.4194303) then
       write(*,1040) trim(callsign), got10, got12, got22
1040   format('Hash range failure for "',a,'"; got ',i0,1x,i0,1x,i0)
       error stop 1
    endif

    ntests=ntests+1
  end subroutine expect_hash_vector

end program test_packjt77_hash_vectors
