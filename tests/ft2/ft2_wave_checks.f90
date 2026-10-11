program ft2_wave_checks
  use wavhdr
  implicit none
  integer, parameter :: n48=105*4*576, n12=90000
  integer tones(103), i, j, nsps, count, offset
  integer seed_size
  integer, allocatable :: seed(:)
  integer*1 bits(77)
  integer*2 audio(n12)
  real wave(n48), saved_ft2(n48), saved_ft4(n48), noise, frequency
  complex unused(n48)
  character*37 msg, sent
  character*64 filename
  type(hdr) header

  call random_seed(size=seed_size)
  allocate(seed(seed_size))
  seed=73
  call random_seed(put=seed)

  msg='CQ K1ABC FN42'
  call genft4(msg,0,sent,bits,tones)
  if(sent.ne.msg) stop 1
  ! Exercise the actual 48 kHz transmit generator, including repeated
  ! FT4 -> FT2 -> FT4 -> FT2 switches within the same process.
  call gen_ft4wave(tones,103,2304,48000.0,1500.0,unused,wave,0,n48)
  saved_ft4=wave
  call gen_ft4wave(tones,103,1152,48000.0,1500.0,unused,wave,0,n48)
  saved_ft2=wave
  if(any(abs(wave(120961:)).gt.0.0)) stop 2
  call gen_ft4wave(tones,103,2304,48000.0,1500.0,unused,wave,0,n48)
  if(maxval(abs(wave-saved_ft4)).gt.1.0e-6) stop 3
  call gen_ft4wave(tones,103,1152,48000.0,1500.0,unused,wave,0,n48)
  if(maxval(abs(wave-saved_ft2)).gt.1.0e-6) stop 4
  ! Deterministic noisy WAVs at several offsets across the receive band.
  ! These files are fed to jt9, exercising the production decoder dispatch,
  ! time stretching, frequency conversion and formatted decode callback.
  do j=1,4
     nsps=1152
     frequency=1500.0
     msg='CQ K1ABC FN42'
     offset=1800
     if(j.eq.2) then
        frequency=500.0
        msg='K1ABC W9XYZ -10'
        offset=4200
     elseif(j.eq.3) then
        frequency=2900.0
        msg='K1ABC W9XYZ RR73'
        offset=600
     elseif(j.eq.4) then
        nsps=2304
        msg='CQ W9XYZ EN37'
        offset=3600
     endif
     call genft4(msg,0,sent,bits,tones)
     if(sent.ne.msg) stop 5
     call gen_ft4wave(tones,103,nsps,48000.0,frequency,unused,wave,0,n48)
     count=45000
     if(j.eq.4) count=90000
     audio=0
     do i=1,count
        ! Seeded broadband noise prevents a zero noise floor.
        call random_number(noise)
        noise=(noise-0.5)*600.0
        if(i.gt.offset.and.i.le.offset+105*nsps/4) then
           audio(i)=nint(10000.0*wave(4*(i-offset)-3)+noise)
        else
           audio(i)=nint(noise)
        endif
     enddo
     write(filename,'(a,i1,a)') 'generated',j,'_000000.wav'
     header=default_header(12000,count)
     open(10,file=trim(filename),status='replace',access='stream')
     write(10) header,audio(1:count)
     close(10)
  enddo
  print *, 'FT2/FT4 transmit waveforms and mode-switch checks passed.'
end program ft2_wave_checks
