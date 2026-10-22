program reist16_wave_checks
  use wavhdr
  implicit none
  integer :: i,j,ok,offset,seed_size
  integer, allocatable :: seed(:)
  integer*2 :: audio(180000)
  real :: wave(614400),frequency,noise
  character(len=37) :: message,sent
  character(len=64) :: filename
  type(hdr) :: header
  call random_seed(size=seed_size)
  allocate(seed(seed_size))
  seed=116
  call random_seed(put=seed)
  do j=1,6
    select case(j)
    case(1)
      message='CQ OE3SRO JN87'
    case(2)
      message='OE3SRO DL1ABC JO31'
    case(3)
      message='DL1ABC OE3SRO -12'
    case(4)
      message='OE3SRO DL1ABC R-10'
    case(5)
      message='DL1ABC OE3SRO RR73'
    case(6)
      message='OE3SRO DL1ABC 73'
    end select
    frequency=300.375+(j-1)*500
    offset=3600+1200*j
    call gen_reist16(message,sent,frequency,j,wave,ok)
    if(ok.ne.1.or.sent.ne.message) stop 1
    if(maxval(abs(wave)).lt.0.7.or.abs(wave(1)).gt.0.001) stop 2
    do i=1,180000
      call random_number(noise)
      noise=(noise-.5)*1200
      audio(i)=nint(noise)
      if(i.gt.offset.and.i.le.offset+153600) audio(i)=nint(noise+5000*wave(4*(i-offset)-3))
    enddo
    write(filename,'(a,i1,a)') 'reist16_',j,'_000000.wav'
    header=default_header(12000,180000)
    open(10,file=trim(filename),status='replace',access='stream')
    write(10) header,audio
    close(10)
  enddo
  message='TEXT THAT WILL NOT FIT INTO THIS MODE'
  call gen_reist16(message,sent,1500.0,0,wave,ok)
  if(ok.ne.0.or.any(wave.ne.0)) stop 3
  print *, 'PASS REIST16: six QSO waveforms and unsupported-message rejection.'
end program
