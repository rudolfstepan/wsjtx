subroutine gen_reist16(message,msgsent,frequency,sequence,wave,ok)
  use iso_c_binding
  use packjt77, only: pack77, unpack77
  implicit none
  character(len=37), intent(in) :: message
  character(len=37), intent(out) :: msgsent
  real(c_float), intent(in) :: frequency
  integer(c_int), intent(in) :: sequence
  real(c_float), intent(out) :: wave(614400)
  integer(c_int), intent(out) :: ok
  character(len=77) :: packed
  integer(c_int8_t) :: bits(77)
  integer :: i3,n3,i,n
  logical :: valid
  interface
    function encode(bits,seq,freq,out,capacity) bind(C,name='reist16_qso2_encode') result(n)
      import
      integer(c_int8_t), intent(in) :: bits(77)
      integer(c_int), value :: seq,capacity
      real(c_float), value :: freq
      real(c_float), intent(out) :: out(*)
      integer(c_int) :: n
    end function
  end interface
  ok=0
  wave=0
  i3=-1
  n3=-1
  call pack77(message,i3,n3,packed)
  call unpack77(packed,0,msgsent,valid)
  if (.not.valid .or. (i3.ne.1 .and. i3.ne.2) .or. msgsent.ne.message) then
    msgsent='*** bad message ***'
    return
  endif
  do i=1,77
    bits(i)=iachar(packed(i:i))-iachar('0')
  enddo
  n=encode(bits,sequence,frequency,wave,614400)
  if(n.eq.614400) ok=1
end subroutine

subroutine decode_reist16(id2,npts,nutc,nfa,nfb,depth,cq_ap,ios13,decoded)
  use iso_c_binding
  use packjt77, only: unpack77
  implicit none
  integer(c_int16_t), intent(in) :: id2(*)
  integer, intent(in) :: npts,nutc,nfa,nfb,depth,ios13
  logical, intent(in) :: cq_ap
  integer, intent(out) :: decoded
  ! Matches reist16::max_decodes; exercised through the production jt9 path.
  integer, parameter :: max_decodes=200
  integer(c_int8_t) :: bits(77,max_decodes)
  real(c_float) :: freq(max_decodes),dt(max_decodes),snr(max_decodes)
  character(len=77) :: packed
  character(len=37) :: message
  integer :: n,i,j,i3
  logical :: valid
  interface
    function decode(samples,count,low,high,level,bits,freq,dt,snr,capacity,ap) &
        bind(C,name='reist16_decode_ex') result(n)
      import
      integer(c_int16_t), intent(in) :: samples(*)
      integer(c_int), value :: count,low,high,level,capacity,ap
      integer(c_int8_t), intent(out) :: bits(77,*)
      real(c_float), intent(out) :: freq(*),dt(*),snr(*)
      integer(c_int) :: n
    end function
  end interface
  decoded=0
  n=decode(id2,min(npts,180000),nfa,nfb,iand(depth,3),bits,freq,dt,snr,max_decodes,merge(1,0,cq_ap))
  do i=1,n
    do j=1,77
      packed(j:j)=achar(iachar('0')+bits(j,i))
    enddo
    read(packed(75:77),'(b3)') i3
    if(i3.ne.1 .and. i3.ne.2) cycle
    call unpack77(packed,1,message,valid)
    if(.not.valid) cycle
    decoded=decoded+1
    write(*,1001) nutc,nint(snr(i)),dt(i),nint(freq(i)),message
1001 format(i6.6,i4,f5.1,i5,' % ',1x,a37)
    if(ios13.eq.0) write(13,1002) nutc,nint(snr(i)),dt(i),freq(i),message
1002 format(i6.6,i5,f6.1,f8.1,3x,a37,' REIST16')
  enddo
  call flush(6)
  if(ios13.eq.0) flush(13)
end subroutine
