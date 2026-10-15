program ft8_wave_checks
  use wavhdr
  implicit none
  integer, parameter :: n48=79*7680,n12=180000
  integer ft8_thread_budget,cores,requested,lim,n,i,j,seed_size
  integer, allocatable :: seed(:)
  integer tones(79),offset,i3,n3
  integer*1 bits(77)
  integer*2 audio(n12)
  real wave(n48),noise,f0
  complex unused(n48)
  character*37 msg,sent
  character*40 filename
  type(hdr) header
  do cores=1,128
     do requested=0,24
        do lim=1,16
           n=ft8_thread_budget(cores,requested,lim)
           if(n.lt.1.or.n.gt.min(12,cores,lim)) stop 1
        enddo
     enddo
  enddo
  if(ft8_thread_budget(16,0,16).ne.12) stop 2
  if(ft8_thread_budget(8,0,8).ne.6) stop 3
  if(ft8_thread_budget(32,12,2).ne.2) stop 4
  call random_seed(size=seed_size)
  allocate(seed(seed_size))
  seed=991
  call random_seed(put=seed)
  msg='CQ K1ABC FN42'
  i3=0
  n3=0
  call genft8(msg,i3,n3,sent,bits,tones)
  if(sent.ne.msg) stop 5
  do j=1,3
     f0=500.0
     if(j.eq.2) f0=1855.0
     if(j.eq.3) f0=2900.0
     call gen_ft8wave(tones,79,7680,2.0,48000.0,f0,unused,wave,0,n48)
     if(maxval(abs(wave)).gt.1.000001) stop 6
     if(abs(wave(1)).gt.0.00001.or.abs(wave(n48)).gt.0.0001) stop 7
     offset=6000
     do i=1,n12
        call random_number(noise)
        noise=(noise-0.5)*600.0
        if(i.gt.offset.and.i.le.offset+n48/4) then
           audio(i)=nint(10000.0*wave(4*(i-offset)-3)+noise)
        else
           audio(i)=nint(noise)
        endif
     enddo
     write(filename,'(a,i1,a)') 'ft8_',j,'_000000.wav'
     header=default_header(12000,n12)
     open(10,file=trim(filename),status='replace',access='stream')
     write(10) header,audio
     close(10)
  enddo
  print *, 'FT8 waveform and SMP thread-budget checks passed.'
end program ft8_wave_checks
